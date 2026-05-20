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
ConvolverNode::ConvolverNode(
    const std::shared_ptr<BaseAudioContext> &context,
    const ConvolverOptions &options)
    : AudioNode(context, options),
      gainCalibrationSampleRate_(context->getSampleRate()),
      internalBufferIndex_(0),
      scaleFactor_(1.0f),
      intermediateBuffer_(nullptr),
      buffer_(nullptr),
      internalBuffer_(nullptr) {}

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

  if (buffer_ != nullptr) {
    context->getDisposer()->dispose(std::move(buffer_));
  }

  if (threadPool_ != nullptr) {
    context->getDisposer()->dispose(std::move(threadPool_));
  }

  for (auto &convolver : convolvers_) {
    context->getDisposer()->dispose(std::move(convolver));
  }

  if (internalBuffer_ != nullptr) {
    context->getDisposer()->dispose(std::move(internalBuffer_));
  }

  if (intermediateBuffer_ != nullptr) {
    context->getDisposer()->dispose(std::move(intermediateBuffer_));
  }

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

  return power;
}

// processing pipeline: audioBuffer_ (input) -> intermediateBuffer_ -> audioBuffer_ (output)
void ConvolverNode::processNode(int framesToProcess) {
  if (buffer_ == nullptr) {
    return;
  }

  if (framesToProcess != RENDER_QUANTUM_SIZE) {
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

  // Once the base-class tail counter has fully drained, stop convolving and
  // emit silence; the IR's contribution has decayed beyond audibility.
  if (tailState_ == TailState::FINISHED) {
    audioBuffer_->zero();
    internalBufferIndex_ = 0;
    return;
  }

  if (internalBufferIndex_ < framesToProcess) {
    performConvolution(audioBuffer_); // reads from audioBuffer_, result goes to intermediateBuffer_
    audioBuffer_->zero();
    audioBuffer_->sum(*intermediateBuffer_);

    internalBuffer_->copy(*audioBuffer_, 0, internalBufferIndex_, RENDER_QUANTUM_SIZE);
    internalBufferIndex_ += RENDER_QUANTUM_SIZE;
  }

  audioBuffer_->zero();
  audioBuffer_->copy(*internalBuffer_, 0, 0, framesToProcess);
  auto remainingFrames = static_cast<int>(internalBufferIndex_ - framesToProcess);
  if (remainingFrames > 0) {
    for (size_t ch = 0; ch < internalBuffer_->getNumberOfChannels(); ++ch) {
      internalBuffer_->getChannel(ch)->copyWithin(framesToProcess, 0, remainingFrames);
    }
  }

  internalBufferIndex_ -= framesToProcess;

  for (int i = 0; i < audioBuffer_->getNumberOfChannels(); ++i) {
    audioBuffer_->getChannel(i)->scale(scaleFactor_);
  }
}

int ConvolverNode::computeTailFrames() const {
  // The convolver's impulse response equals the IR buffer itself, so a full
  // tail equals one IR length of samples. If no IR has been set yet, there
  // is nothing to ring out.
  return buffer_ ? static_cast<int>(buffer_->getSize()) : 0;
}

void ConvolverNode::performConvolution(const std::shared_ptr<DSPAudioBuffer> &processingBuffer) {
  if (processingBuffer->getNumberOfChannels() == 1) {
    for (int i = 0; i < convolvers_.size(); ++i) {
      threadPool_->schedule([&, i] {
        convolvers_[i]->process(
            *processingBuffer->getChannel(0), *intermediateBuffer_->getChannel(i));
      });
    }
  } else if (processingBuffer->getNumberOfChannels() == 2) {
    if (convolvers_.size() == 2) {
      inputChannelMap_ = {0, 1, 0, 0};
      outputChannelMap_ = {0, 1, 0, 0};
    } else { // 4 channel IR
      inputChannelMap_ = {0, 0, 1, 1};
      outputChannelMap_ = {0, 3, 2, 1};
    }
    for (int i = 0; i < convolvers_.size(); ++i) {
      threadPool_->schedule([this, i, &processingBuffer] {
        convolvers_[i]->process(
            *processingBuffer->getChannel(inputChannelMap_[i]),
            *intermediateBuffer_->getChannel(outputChannelMap_[i]));
      });
    }
  }
  threadPool_->wait();
}
} // namespace audioapi
