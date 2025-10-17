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
    std::shared_ptr<AudioBuffer> buffer,
    bool disableNormalization)
    : AudioNode(context),
      normalize_(true),
      buffer_(nullptr),
      internalBuffer_(nullptr) {
  channelCount_ = 2;
  channelCountMode_ = ChannelCountMode::CLAMPED_MAX;
  normalize_ = !disableNormalization;
  audioBus_ = std::make_shared<AudioBus>(
      RENDER_QUANTUM_SIZE, channelCount_, context->getSampleRate());
  gainCalibrationSampleRate_ = context->getSampleRate();
  setBuffer(buffer);
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
    convolvers_.clear();
    for (int i = 0; i < 2; ++i) {
      convolvers_.emplace_back();
      AudioArray channelData(buffer->getLength());
      int channelNumber = buffer->getNumberOfChannels() == 2 ? i : 0;
      memcpy(
          channelData.getData(),
          buffer->getChannelData(channelNumber),
          buffer->getLength() * sizeof(float));
      convolvers_.back().init(
          RENDER_QUANTUM_SIZE, channelData, buffer->getLength());
    }
    if (buffer->getNumberOfChannels() == 4) {
      for (int i = 2; i < 4; ++i) {
        convolvers_.emplace_back();
        AudioArray channelData(buffer->getLength());
        memcpy(
            channelData.getData(),
            buffer->getChannelData(i),
            buffer->getLength() * sizeof(float));
        convolvers_.back().init(
            RENDER_QUANTUM_SIZE, channelData, buffer->getLength());
      }
    }
    internalBuffer_ = std::make_shared<AudioBus>(
        RENDER_QUANTUM_SIZE * 2, channelCount_, buffer->getSampleRate());
  }
}

void ConvolverNode::onInputDisabled() {
  numberOfEnabledInputNodes_ -= 1;
  if (isEnabled() && numberOfEnabledInputNodes_ == 0) {
    signaledToStop_ = true;
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

std::shared_ptr<AudioBus> ConvolverNode::processNode(
    const std::shared_ptr<AudioBus> &processingBus,
    int framesToProcess) {
  if (signaledToStop_) {
    if (remainingSegments_ > 0) {
      remainingSegments_--;
    } else {
      disable();
      signaledToStop_ = false;
      internalBufferIndex_ = 0;
      return processingBus;
    }
  }
  if (internalBufferIndex_ < framesToProcess) {
    performConvolution(processingBus);

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

  for (int i = 0; i < processingBus->getNumberOfChannels(); ++i) {
    dsp::multiplyByScalar(
        processingBus->getChannel(i)->getData(),
        scaleFactor_,
        processingBus->getChannel(i)->getData(),
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
  if (power < minPower_) {
    power = minPower_;
  }
  scaleFactor_ = 1 / power;
  scaleFactor_ *= std::pow(10, gainCalibration_ * 0.05f);
  scaleFactor_ *= gainCalibrationSampleRate_ / buffer_->getSampleRate();

  if (numberOfChannels == 4)
    scaleFactor_ *= 0.5;
}

void ConvolverNode::performConvolution(
    const std::shared_ptr<AudioBus> &processingBus) {
  std::vector<std::thread> threads;
  if (processingBus->getNumberOfChannels() == 1) {
    if (convolvers_.size() == 1) {
      convolvers_[0].process(
          processingBus->getChannel(0)->getData(),
          audioBus_->getChannel(0)->getData());
    } else if (convolvers_.size() == 2) {
      for (int i = 0; i < convolvers_.size(); ++i) {
        threads.emplace_back([this, i, processingBus]() {
          convolvers_[i].process(
              processingBus->getChannel(0)->getData(),
              audioBus_->getChannel(i)->getData());
        });
      }
    } else { // convolvers.size() == 4
      for (int i = 0; i < 2; ++i) {
        threads.emplace_back([this, i, processingBus]() {
          convolvers_[i].process(
              processingBus->getChannel(0)->getData(),
              audioBus_->getChannel(i)->getData());
        });
      }
      threads.emplace_back([this, processingBus]() {
        convolvers_[2].process(
            processingBus->getChannel(0)->getData(), thirdChannelData_);
      });
      threads.emplace_back([this, processingBus]() {
        convolvers_[3].process(
            processingBus->getChannel(0)->getData(), fourthChannelData_);
      });
    }
  } else if (processingBus->getNumberOfChannels() == 2) {
    if (convolvers_.size() == 2) {
      for (int i = 0; i < 2; ++i) {
        threads.emplace_back([this, i, processingBus]() {
          convolvers_[i].process(
              processingBus->getChannel(i)->getData(),
              audioBus_->getChannel(i)->getData());
        });
      }
    } else { // convolvers.size() == 4
      threads.emplace_back([this, processingBus]() {
        convolvers_[0].process(
            processingBus->getChannel(0)->getData(),
            audioBus_->getChannel(0)->getData());
      });
      threads.emplace_back([this, processingBus]() {
        convolvers_[1].process(
            processingBus->getChannel(0)->getData(), fourthChannelData_);
      });
      threads.emplace_back([this, processingBus]() {
        convolvers_[2].process(
            processingBus->getChannel(1)->getData(), thirdChannelData_);
      });
      threads.emplace_back([this, processingBus]() {
        convolvers_[3].process(
            processingBus->getChannel(1)->getData(),
            audioBus_->getChannel(1)->getData());
      });
    }
  }
  if (!threads.empty()) {
    for (auto &thread : threads) {
      thread.join();
    }
  }
  if (convolvers_.size() == 4) {
    dsp::add(
        audioBus_->getChannel(0)->getData(),
        thirdChannelData_,
        audioBus_->getChannel(0)->getData(),
        RENDER_QUANTUM_SIZE);
    dsp::multiplyByScalar(
        audioBus_->getChannel(0)->getData(),
        0.5f,
        audioBus_->getChannel(0)->getData(),
        RENDER_QUANTUM_SIZE);
    dsp::add(
        audioBus_->getChannel(1)->getData(),
        fourthChannelData_,
        audioBus_->getChannel(1)->getData(),
        RENDER_QUANTUM_SIZE);
    dsp::multiplyByScalar(
        audioBus_->getChannel(1)->getData(),
        0.5f,
        audioBus_->getChannel(1)->getData(),
        RENDER_QUANTUM_SIZE);
  }
}
} // namespace audioapi
