#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/effects/ConvolverNode.h>
#include <audioapi/core/utils/AudioGraphManager.h>
#include <audioapi/core/utils/Constants.h>
#include <audioapi/types/NodeOptions.h>
#include <audioapi/utils/AudioArray.hpp>

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
      remainingSegments_(0),
      internalBufferIndex_(0),
      signalledToStop_(false),
      scaleFactor_(1.0f),
      intermediateBuffer_(nullptr),
      buffer_(nullptr),
      internalBuffer_(nullptr) {
  isInitialized_.store(true, std::memory_order_release);
}

void ConvolverNode::setBuffer(
    const std::shared_ptr<AudioBuffer> &buffer,
    std::vector<std::unique_ptr<Convolver>> convolvers,
    const std::shared_ptr<ThreadPool> &threadPool,
    const std::shared_ptr<DSPAudioBuffer> &internalBuffer,
    const std::shared_ptr<DSPAudioBuffer> &intermediateBuffer,
    float scaleFactor) {
  std::shared_ptr<BaseAudioContext> context = context_.lock();
  if (context == nullptr) {
    return;
  }

  auto graphManager = context->getGraphManager();

  if (buffer_) {
    graphManager->addAudioBufferForDestruction(std::move(buffer_));
  }

  // TODO move convolvers, thread pool and DSPAudioBuffers destruction to graph manager as well

  buffer_ = buffer;
  convolvers_ = std::move(convolvers);
  threadPool_ = threadPool;
  internalBuffer_ = internalBuffer;
  intermediateBuffer_ = intermediateBuffer;
  scaleFactor_ = scaleFactor;
  internalBufferIndex_ = 0;
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

void ConvolverNode::onInputDisabled() {
  numberOfEnabledInputNodes_ -= 1;
  if (isEnabled() && numberOfEnabledInputNodes_ == 0) {
    signalledToStop_ = true;
    remainingSegments_ = convolvers_.at(0)->getSegCount();
  }
}

// processing pipeline: processingBuffer -> intermediateBuffer_ -> audioBuffer_ (mixing
// with intermediateBuffer_)
std::shared_ptr<DSPAudioBuffer> ConvolverNode::processNode(
    const std::shared_ptr<DSPAudioBuffer> &processingBuffer,
    int framesToProcess) {
  if (processingBuffer->getSize() != RENDER_QUANTUM_SIZE) {
    printf(
        "[AUDIOAPI WARN] convolver requires 128 buffer size for each render quantum, otherwise quality of convolution is very poor\n");
  }
  if (signalledToStop_) {
    if (remainingSegments_ > 0) {
      remainingSegments_--;
    } else {
      disable();
      signalledToStop_ = false;
      internalBufferIndex_ = 0;
      return processingBuffer;
    }
  }
  if (internalBufferIndex_ < framesToProcess) {
    performConvolution(processingBuffer); // result returned to intermediateBuffer_
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

  return audioBuffer_;
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
    std::vector<int> inputChannelMap;
    std::vector<int> outputChannelMap;
    if (convolvers_.size() == 2) {
      inputChannelMap = {0, 1};
      outputChannelMap = {0, 1};
    } else { // 4 channel IR
      inputChannelMap = {0, 0, 1, 1};
      outputChannelMap = {0, 3, 2, 1};
    }
    for (int i = 0; i < convolvers_.size(); ++i) {
      threadPool_->schedule([this, i, inputChannelMap, outputChannelMap, &processingBuffer] {
        convolvers_[i]->process(
            *processingBuffer->getChannel(inputChannelMap[i]),
            *intermediateBuffer_->getChannel(outputChannelMap[i]));
      });
    }
  }
  threadPool_->wait();
}
} // namespace audioapi
