#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/effects/ConvolverNode.h>
#include <audioapi/core/sources/AudioBuffer.h>
#include <audioapi/core/utils/Constants.h>
#include <audioapi/dsp/AudioUtils.h>
#include <audioapi/dsp/FFT.h>
#include <audioapi/utils/AudioArray.h>
#include <iostream>
#include <thread>

namespace audioapi {
ConvolverNode::ConvolverNode(
    BaseAudioContext *context,
    const std::shared_ptr<AudioBuffer> &buffer,
    bool disableNormalization)
    : AudioNode(context),
      remainingSegments_(0),
      internalBufferIndex_(0),
      signalledToStop_(false),
      scaleFactor_(1.0f),
      intermediateBus_(nullptr),
      buffer_(nullptr),
      internalBuffer_(nullptr) {
  channelCount_ = 2;
  channelCountMode_ = ChannelCountMode::CLAMPED_MAX;
  normalize_ = !disableNormalization;
  gainCalibrationSampleRate_ = context->getSampleRate();
  setBuffer(buffer);
  audioBus_ = std::make_shared<AudioBus>(
      RENDER_QUANTUM_SIZE, channelCount_, context->getSampleRate());
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
    for (int i = 0; i < buffer->getNumberOfChannels(); ++i) {
      convolvers_.emplace_back();
      AudioArray channelData(buffer->getLength());
      memcpy(
          channelData.getData(),
          buffer->getChannelData(i),
          buffer->getLength() * sizeof(float));
      convolvers_.back().init(
          RENDER_QUANTUM_SIZE, channelData, buffer->getLength());
    }
    if (buffer->getNumberOfChannels() == 1) {
      // add one more convolver, because right now input is always stereo
      convolvers_.emplace_back();
      AudioArray channelData(buffer->getLength());
      memcpy(
          channelData.getData(),
          buffer->getChannelData(0),
          buffer->getLength() * sizeof(float));
      convolvers_.back().init(
          RENDER_QUANTUM_SIZE, channelData, buffer->getLength());
    }
    internalBuffer_ = std::make_shared<AudioBus>(
        RENDER_QUANTUM_SIZE * 2, channelCount_, buffer->getSampleRate());
    intermediateBus_ = std::make_shared<AudioBus>(
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

std::shared_ptr<AudioBus> ConvolverNode::processInputs(
    const std::shared_ptr<AudioBus> &outputBus,
    int framesToProcess,
    bool checkIsAlreadyProcessed) {
  if (internalBufferIndex_ < framesToProcess) {
    return AudioNode::processInputs(outputBus, RENDER_QUANTUM_SIZE, false);
  }
  return AudioNode::processInputs(outputBus, 0, false);
}

// processing pipeline: processingBus -> intermediateBus_ -> audioBus_ (mixing
// with intermediateBus_)
std::shared_ptr<AudioBus> ConvolverNode::processNode(
    const std::shared_ptr<AudioBus> &processingBus,
    int framesToProcess) {
  if (signalledToStop_) {
    if (remainingSegments_ > 0) {
      remainingSegments_--;
    } else {
      disable();
      signalledToStop_ = false;
      internalBufferIndex_ = 0;
      return processingBus;
    }
  }
  if (internalBufferIndex_ < framesToProcess) {
    performConvolution(processingBus); // result returned to intermediateBus_
    audioBus_->sum(intermediateBus_.get());

    internalBuffer_->copy(
        audioBus_.get(), 0, internalBufferIndex_, RENDER_QUANTUM_SIZE);
    internalBufferIndex_ += RENDER_QUANTUM_SIZE;
  }
  audioBus_->zero();
  audioBus_->copy(internalBuffer_.get(), 0, 0, framesToProcess);
  int remainingFrames = internalBufferIndex_ - framesToProcess;
  if (remainingFrames > 0) {
    for (int i = 0; i < internalBuffer_->getNumberOfChannels(); ++i) {
      memmove(
          internalBuffer_->getChannel(i)->getData(),
          internalBuffer_->getChannel(i)->getData() + framesToProcess,
          remainingFrames * sizeof(float));
    }
  }
  internalBufferIndex_ -= framesToProcess;

  for (int i = 0; i < audioBus_->getNumberOfChannels(); ++i) {
    dsp::multiplyByScalar(
        audioBus_->getChannel(i)->getData(),
        scaleFactor_,
        audioBus_->getChannel(i)->getData(),
        framesToProcess);
  }

  return audioBus_;
}

void ConvolverNode::calculateNormalizationScale() {
  int numberOfChannels = buffer_->getNumberOfChannels();
  int length = buffer_->getLength();

  float power = 0;

  for (int channel = 0; channel < numberOfChannels; ++channel) {
    float channelPower = 0;
    auto channelData = buffer_->getChannelData(channel);
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

void ConvolverNode::performConvolution(
    const std::shared_ptr<AudioBus> &processingBus) {
  if (processingBus->getNumberOfChannels() == 1) {
    for (int i = 0; i < convolvers_.size(); ++i) {
      threadPool_->schedule([&, i] {
        convolvers_[i].process(
            processingBus->getChannel(0)->getData(),
            intermediateBus_->getChannel(i)->getData());
      });
    }
  } else if (processingBus->getNumberOfChannels() == 2) {
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
      threadPool_->schedule(
          [this, i, inputChannelMap, outputChannelMap, &processingBus] {
            convolvers_[i].process(
                processingBus->getChannel(inputChannelMap[i])->getData(),
                intermediateBus_->getChannel(outputChannelMap[i])->getData());
          });
    }
  }
  threadPool_->wait();
}
} // namespace audioapi
