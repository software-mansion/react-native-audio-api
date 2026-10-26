#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/effects/ConvolverNode.h>
#include <audioapi/core/utils/Constants.h>
#include <audioapi/types/NodeOptions.h>
#include <audioapi/utils/AudioArray.hpp>

#include <algorithm>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

namespace audioapi {
ConvolverNode::ConvolverNode(
    const std::shared_ptr<BaseAudioContext> &context,
    const ConvolverOptions &options)
    : AudioNode(context, options),
      gainCalibrationSampleRate_(context->getSampleRate()),
      scaleFactor_(1.0f),
      intermediateBuffer_(nullptr),
      buffer_(nullptr) {}

void ConvolverNode::setBuffer(
    const std::shared_ptr<AudioBuffer> &buffer,
    std::vector<std::unique_ptr<Convolver>> convolvers,
    const std::shared_ptr<ConvolverThreadPool> &threadPool,
    const std::shared_ptr<DSPAudioBuffer> &intermediateBuffer,
    float scaleFactor) {
  std::shared_ptr<BaseAudioContext> context = context_.lock();
  if (context == nullptr) {
    return;
  }

  if (buffer_ != nullptr) {
    context->getDisposer()->dispose(std::move(buffer_));
  }

  for (auto &convolver : convolvers_) {
    context->getDisposer()->dispose(std::move(convolver));
  }

  if (intermediateBuffer_ != nullptr) {
    context->getDisposer()->dispose(std::move(intermediateBuffer_));
  }

  buffer_ = buffer;
  convolvers_ = std::move(convolvers);
  threadPool_ = threadPool;
  intermediateBuffer_ = intermediateBuffer;
  scaleFactor_ = scaleFactor;

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

  // Once the base-class tail counter has fully drained, stop convolving and
  // emit silence; the IR's contribution has decayed beyond audibility.
  if (tailState_ == TailState::FINISHED) {
    audioBuffer_->zero();
    return;
  }

  // Convolvers write only `framesToProcess` frames; clear the rest so a short quantum does not
  // carry stale frames into the sum below.
  intermediateBuffer_->zero();
  performConvolution(audioBuffer_, framesToProcess);
  audioBuffer_->zero();
  audioBuffer_->sum(*intermediateBuffer_);

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

void ConvolverNode::performConvolution(
    const std::shared_ptr<DSPAudioBuffer> &processingBuffer,
    int framesToProcess) {
  // 32 bits keeps each task within the pool's fixed task storage.
  const auto frames = static_cast<uint32_t>(framesToProcess);
  if (processingBuffer->getNumberOfChannels() == 1) {
    for (int i = 0; i < convolvers_.size(); ++i) {
      threadPool_->schedule([this, &processingBuffer, i, frames] {
        convolvers_[i]->process(
            *processingBuffer->getChannel(0), *intermediateBuffer_->getChannel(i), frames);
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
      threadPool_->schedule([this, &processingBuffer, i, frames] {
        convolvers_[i]->process(
            *processingBuffer->getChannel(inputChannelMap_[i]),
            *intermediateBuffer_->getChannel(outputChannelMap_[i]),
            frames);
      });
    }
  }
  threadPool_->wait();
}
} // namespace audioapi
