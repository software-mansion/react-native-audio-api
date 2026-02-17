#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/effects/ConvolverNode.h>
#include <audioapi/core/utils/Constants.h>
#include <audioapi/dsp/AudioUtils.hpp>
#include <audioapi/dsp/FFT.h>
#include <audioapi/types/NodeOptions.h>
#include <audioapi/utils/AudioArray.h>
#include <audioapi/utils/AudioBuffer.h>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>

namespace audioapi {
ConvolverNode::ConvolverNode(
    const std::shared_ptr<BaseAudioContext> &context,
    const ConvolverOptions &options)
    : AudioNode(context, options),
      gainCalibrationSampleRate_(context->getSampleRate()),
      remainingSegments_(0),
      internalBufferIndex_(0),
      normalize_(!options.disableNormalization),
      signalledToStop_(false),
      scaleFactor_(1.0f),
      intermediateBuffer_(nullptr),
      buffer_(nullptr),
      internalBuffer_(nullptr) {
  setBuffer(options.buffer);
  isInitialized_ = true;
}

bool ConvolverNode::getNormalize_() const {
  return normalize_;
}

const std::shared_ptr<AudioBuffer> &ConvolverNode::getBuffer() const {
  return buffer_;
}

void ConvolverNode::setNormalize(bool normalize) {
  if (normalize_ != normalize) {
    normalize_ = normalize;
    if (normalize_ && buffer_)
      calculateNormalizationScale();
  }
  if (!normalize_) {
    scaleFactor_ = 1.0f;
  }
}

void ConvolverNode::setBuffer(const std::shared_ptr<AudioBuffer> &buffer) {
  if (buffer_ != buffer && buffer != nullptr) {
    buffer_ = buffer;
    if (normalize_)
      calculateNormalizationScale();
    threadPool_ = std::make_shared<ThreadPool>(4);
    convolvers_.clear();
    for (size_t i = 0; i < buffer->getNumberOfChannels(); ++i) {
      convolvers_.emplace_back();
      AudioArray channelData(*buffer->getChannel(i));
      convolvers_.back().init(RENDER_QUANTUM_SIZE, channelData, buffer->getSize());
    }
    if (buffer->getNumberOfChannels() == 1) {
      // add one more convolver, because right now input is always stereo
      convolvers_.emplace_back();
      AudioArray channelData(*buffer->getChannel(0));
      convolvers_.back().init(RENDER_QUANTUM_SIZE, channelData, buffer->getSize());
    }
    internalBuffer_ = std::make_shared<AudioBuffer>(
        RENDER_QUANTUM_SIZE * 2, channelCount_, buffer->getSampleRate());
    intermediateBuffer_ = std::make_shared<AudioBuffer>(
        RENDER_QUANTUM_SIZE, convolvers_.size(), buffer->getSampleRate());
    internalBufferIndex_ = 0;
  }
}

void ConvolverNode::onInputDisabled() {
  numberOfEnabledInputNodes_ -= 1;
  if (isEnabled() && numberOfEnabledInputNodes_ == 0) {
    signalledToStop_ = true;
    remainingSegments_ = convolvers_.at(0).getSegCount();
  }
}

std::shared_ptr<AudioBuffer> ConvolverNode::processInputs(
    const std::shared_ptr<AudioBuffer> &outputBuffer,
    int framesToProcess,
    bool checkIsAlreadyProcessed) {
  if (internalBufferIndex_ < framesToProcess) {
    return AudioNode::processInputs(outputBuffer, RENDER_QUANTUM_SIZE, false);
  }
  return AudioNode::processInputs(outputBuffer, 0, false);
}

// processing pipeline: processingBuffer -> intermediateBuffer_ -> audioBuffer_ (mixing
// with intermediateBuffer_)
std::shared_ptr<AudioBuffer> ConvolverNode::processNode(
    const std::shared_ptr<AudioBuffer> &processingBuffer,
    int framesToProcess) {
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
  int remainingFrames = internalBufferIndex_ - framesToProcess;
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

void ConvolverNode::calculateNormalizationScale() {
  int numberOfChannels = buffer_->getNumberOfChannels();
  auto length = buffer_->getSize();

  float power = 0;

  for (size_t channel = 0; channel < numberOfChannels; ++channel) {
    float channelPower = 0;
    auto channelData = buffer_->getChannel(channel)->span();
    for (int i = 0; i < length; ++i) {
      float sample = channelData[i];
      channelPower += sample * sample;
    }
    power += channelPower;
  }

  power = std::sqrt(power / (numberOfChannels * length));
  if (power < MIN_IR_POWER) {
    power = MIN_IR_POWER;
  }
  scaleFactor_ = 1 / power;
  scaleFactor_ *= std::pow(10, GAIN_CALIBRATION * 0.05f);
  scaleFactor_ *= gainCalibrationSampleRate_ / buffer_->getSampleRate();
}

void ConvolverNode::performConvolution(const std::shared_ptr<AudioBuffer> &processingBuffer) {
  if (processingBuffer->getNumberOfChannels() == 1) {
    for (int i = 0; i < convolvers_.size(); ++i) {
      threadPool_->schedule([&, i] {
        convolvers_[i].process(
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
        convolvers_[i].process(
            *processingBuffer->getChannel(inputChannelMap[i]),
            *intermediateBuffer_->getChannel(outputChannelMap[i]));
      });
    }
  }
  threadPool_->wait();
}
} // namespace audioapi
