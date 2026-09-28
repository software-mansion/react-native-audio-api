#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/effects/ConvolverNode.h>
#include <audioapi/core/utils/Constants.h>
#include <audioapi/types/NodeOptions.h>
#include <audioapi/utils/AudioArray.hpp>

#ifdef ANDROID
#include <android/log.h>
#endif

#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

namespace audioapi {

namespace {

constexpr size_t kMonoChannelCount = 1;
constexpr size_t kStereoChannelCount = 2;
constexpr size_t kTrueStereoImpulseResponseChannelCount = 4;

/// Hands an audio-thread-owned object to the disposer (freed off-thread) and
/// leaves the pointer empty.
template <typename Pointer>
void disposeIfSet(BaseAudioContext &context, Pointer &pointer) {
  if (pointer != nullptr) {
    context.getDisposer()->dispose(std::move(pointer));
  }
}

void warnIfNotRenderQuantum(int framesToProcess) {
  if (framesToProcess == RENDER_QUANTUM_SIZE) {
    return;
  }
#ifdef ANDROID
  __android_log_print(
      ANDROID_LOG_WARN,
      "RN_AUDIOAPI",
      "convolver requires 128 buffer size for each render quantum, otherwise quality of convolution is very poor");
#else
  printf(
      "[RN_AUDIOAPI WARN] convolver requires 128 buffer size for each render quantum, otherwise quality of convolution is very poor\n");
#endif
}

} // namespace

ConvolverNode::ConvolverNode(
    const std::shared_ptr<BaseAudioContext> &context,
    const ConvolverOptions &options)
    : AudioNode(context, options),
      gainCalibrationSampleRate_(context->getSampleRate()),
      internalBufferIndex_(0),
      scaleFactor_(1.0f),
      intermediateBuffer_(nullptr),
      buffer_(nullptr),
      internalBuffer_(nullptr),
      impulseResponseForNegotiation_(nullptr),
      convolversScheduled_(0),
      negotiatedInputChannelCount_(options.channelCount),
      monoMixBuffer_(
          std::make_shared<DSPAudioBuffer>(
              RENDER_QUANTUM_SIZE,
              kMonoChannelCount,
              context->getSampleRate())) {}

void ConvolverNode::releaseImpulseResponse(BaseAudioContext &context) {
  disposeIfSet(context, buffer_);
  disposeIfSet(context, threadPool_);
  for (auto &convolver : convolvers_) {
    disposeIfSet(context, convolver);
  }
  convolvers_.clear();
  disposeIfSet(context, internalBuffer_);
  disposeIfSet(context, intermediateBuffer_);
  internalBufferIndex_ = 0;
}

void ConvolverNode::setBuffer(
    const std::shared_ptr<AudioBuffer> &buffer,
    std::vector<std::unique_ptr<Convolver>> convolvers,
    const std::shared_ptr<ConvolverThreadPool> &threadPool,
    const std::shared_ptr<DSPAudioBuffer> &internalBuffer,
    const std::shared_ptr<DSPAudioBuffer> &intermediateBuffer,
    float scaleFactor) {
  std::shared_ptr<BaseAudioContext> context = context_.lock();
  if (context == nullptr) {
    return;
  }

  releaseImpulseResponse(*context);

  buffer_ = buffer;
  convolvers_ = std::move(convolvers);
  threadPool_ = threadPool;
  internalBuffer_ = internalBuffer;
  intermediateBuffer_ = intermediateBuffer;
  scaleFactor_ = scaleFactor;
  internalBufferIndex_ = 0;

  // Re-arm the tail: a brand-new IR may have a completely different length,
  // and any pending tail countdown from the previous IR is now meaningless.
  // The base-class state machine will recompute computeTailFrames() the next
  // time the input goes silent.
  tailState_ = TailState::ACTIVE;
  tailFramesRemaining_ = 0;
}

void ConvolverNode::clearBuffer() {
  std::shared_ptr<BaseAudioContext> context = context_.lock();
  if (context == nullptr) {
    return;
  }

  releaseImpulseResponse(*context);
  audioBuffer_->zero();
}

void ConvolverNode::appendConvolver(std::unique_ptr<Convolver> &&convolver) {
  convolvers_.push_back(std::move(convolver));
}

void ConvolverNode::setImpulseResponseForNegotiation(
    std::shared_ptr<AudioBuffer> impulseResponse,
    size_t scheduledConvolvers) {
  impulseResponseForNegotiation_ = std::move(impulseResponse);
  convolversScheduled_ = scheduledConvolvers;
}

std::unique_ptr<Convolver> ConvolverNode::makeConvolver(
    const AudioBuffer &impulseResponse,
    size_t channel) {
  AudioArray channelData(*impulseResponse.getChannel(channel));
  auto convolver = std::make_unique<Convolver>();
  convolver->init(RENDER_QUANTUM_SIZE, channelData, impulseResponse.getSize());
  return convolver;
}

float ConvolverNode::calculateNormalizationScale(const std::shared_ptr<AudioBuffer> &buffer) const {
  auto numberOfChannels = buffer->getNumberOfChannels();
  auto length = buffer->getSize();

  float power = 0;

  for (size_t channel = 0; channel < numberOfChannels; ++channel) {
    float channelPower = 0;
    auto channelData = buffer->getChannel(channel)->span();
    for (size_t i = 0; i < length; ++i) {
      float sample = channelData[i];
      channelPower += sample * sample;
    }
    power += channelPower;
  }

  power = std::sqrt(power / (numberOfChannels * length));
  power = std::max(power, MIN_IR_POWER);
  power = 1 / power;
  power *= std::pow(10, GAIN_CALIBRATION * 0.05f);
  power *= gainCalibrationSampleRate_ / buffer->getSampleRate();

  if (numberOfChannels == kTrueStereoImpulseResponseChannelCount) {
    // Spec "true-stereo compensation": each output channel sums two convolutions.
    power *= 0.5f;
  }

  return power;
}

size_t ConvolverNode::outputChannelCountFor(size_t inputChannelCount) const {
  if (impulseResponseForNegotiation_ == nullptr) {
    return kMonoChannelCount;
  }
  const size_t irChannels = impulseResponseForNegotiation_->getNumberOfChannels();
  if (irChannels == kMonoChannelCount && inputChannelCount == kMonoChannelCount) {
    return kMonoChannelCount;
  }
  return kStereoChannelCount;
}

size_t ConvolverNode::getUpstreamChannelCount(size_t negotiatedChannelCount) const {
  return outputChannelCountFor(negotiatedChannelCount);
}

size_t ConvolverNode::negotiateBufferChannelCount(size_t negotiatedChannelCount) {
  negotiatedInputChannelCount_.store(negotiatedChannelCount, std::memory_order_release);

  const bool monoResponse = impulseResponseForNegotiation_ != nullptr &&
      impulseResponseForNegotiation_->getNumberOfChannels() == kMonoChannelCount;
  if (monoResponse && negotiatedChannelCount == kStereoChannelCount &&
      convolversScheduled_ < kStereoChannelCount) {
    convolversScheduled_ = kStereoChannelCount;
    scheduleAudioEvent(
        [this, convolver = makeConvolver(*impulseResponseForNegotiation_, 0)](
            BaseAudioContext & /*context*/) mutable { appendConvolver(std::move(convolver)); });
  }

  return outputChannelCountFor(negotiatedChannelCount);
}

void ConvolverNode::mixInputs(const std::vector<const DSPAudioBuffer *> &inputs) {
  const size_t inputChannels = negotiatedInputChannelCount_.load(std::memory_order_acquire);
  if (inputChannels >= audioBuffer_->getNumberOfChannels()) {
    AudioNode::mixInputs(inputs);
    return;
  }

  monoMixBuffer_->zero();
  for (const DSPAudioBuffer *input : inputs) {
    monoMixBuffer_->sum(*input, getChannelInterpretation());
  }
  for (size_t ch = 0; ch < audioBuffer_->getNumberOfChannels(); ++ch) {
    audioBuffer_->getChannel(ch)->copy(*monoMixBuffer_->getChannel(0));
  }
}

// processing pipeline: audioBuffer_ (input) -> intermediateBuffer_ -> internalBuffer_
void ConvolverNode::renderQuantum() {
  const size_t bufferChannels = audioBuffer_->getNumberOfChannels();
  const bool trueStereo = buffer_->getNumberOfChannels() == kTrueStereoImpulseResponseChannelCount;
  // A mono response runs one convolver per buffer channel, never more than exist.
  const size_t activeConvolvers = buffer_->getNumberOfChannels() == kMonoChannelCount
      ? std::min(bufferChannels, convolvers_.size())
      : convolvers_.size();

  for (size_t i = 0; i < activeConvolvers; ++i) {
    const size_t inputChannel = std::min(trueStereo ? i / 2 : i, bufferChannels - 1);
    threadPool_->schedule([this, i, inputChannel] {
      convolvers_[i]->process(
          *audioBuffer_->getChannel(inputChannel), *intermediateBuffer_->getChannel(i));
    });
  }
  threadPool_->wait();

  for (size_t ch = 0; ch < internalBuffer_->getNumberOfChannels(); ++ch) {
    internalBuffer_->getChannel(ch)->zero(internalBufferIndex_, RENDER_QUANTUM_SIZE);
  }
  for (size_t i = 0; i < activeConvolvers; ++i) {
    const size_t outputChannel = trueStereo ? i % 2 : i;
    if (outputChannel < bufferChannels) {
      internalBuffer_->getChannel(outputChannel)
          ->sum(*intermediateBuffer_->getChannel(i), 0, internalBufferIndex_, RENDER_QUANTUM_SIZE);
    }
  }
}

void ConvolverNode::processNode(int framesToProcess) {
  if (buffer_ == nullptr) {
    // Spec: a convolver without an impulse response outputs silence.
    audioBuffer_->zero();
    return;
  }

  warnIfNotRenderQuantum(framesToProcess);

  // Once the base-class tail counter has fully drained, stop convolving and
  // emit silence; the IR's contribution has decayed beyond audibility.
  if (tailState_ == TailState::FINISHED) {
    audioBuffer_->zero();
    internalBufferIndex_ = 0;
    return;
  }

  if (internalBufferIndex_ < framesToProcess) {
    renderQuantum();
    internalBufferIndex_ += RENDER_QUANTUM_SIZE;
  }

  const size_t outputChannels = std::min(audioBuffer_->getNumberOfChannels(), kStereoChannelCount);
  for (size_t ch = 0; ch < outputChannels; ++ch) {
    audioBuffer_->getChannel(ch)->copy(*internalBuffer_->getChannel(ch), 0, 0, framesToProcess);
  }

  auto remainingFrames = static_cast<int>(internalBufferIndex_ - framesToProcess);
  if (remainingFrames > 0) {
    for (size_t ch = 0; ch < internalBuffer_->getNumberOfChannels(); ++ch) {
      internalBuffer_->getChannel(ch)->copyWithin(framesToProcess, 0, remainingFrames);
    }
  }

  internalBufferIndex_ -= framesToProcess;

  for (size_t ch = 0; ch < outputChannels; ++ch) {
    audioBuffer_->getChannel(ch)->scale(scaleFactor_);
  }
}

int ConvolverNode::computeTailFrames() const {
  // The convolver's impulse response equals the IR buffer itself, so a full
  // tail equals one IR length of samples. If no IR has been set yet, there
  // is nothing to ring out.
  return buffer_ ? static_cast<int>(buffer_->getSize()) : 0;
}

} // namespace audioapi
