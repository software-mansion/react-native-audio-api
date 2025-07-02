#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/effects/ConvolverNode.h>
#include <audioapi/core/sources/AudioBuffer.h>
#include <audioapi/dsp/AudioUtils.h>
#include <audioapi/dsp/FFT.h>
#include <audioapi/utils/AudioArray.h>
#include <iostream>

namespace audioapi {
ConvolverNode::ConvolverNode(BaseAudioContext *context) : AudioNode(context) {
  normalize_ = true;
  buffer_ = std::shared_ptr<AudioBuffer>(nullptr);
  convolver_ = std::shared_ptr<Convolver>(nullptr);
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
  if (buffer_ != buffer) {
    buffer_ = buffer;
    if (normalize_)
      calculateNormalizationScale();
    convolver_ = std::make_shared<Convolver>();
    auto audioArray = AudioArray(buffer->getLength());
    memcpy(
        audioArray.getData(), buffer->getChannelData(0), buffer->getLength());
    convolver_->init(128, audioArray, audioArray.getSize());
  }
}

void ConvolverNode::processNode(
    const std::shared_ptr<AudioBus> &processingBus,
    int framesToProcess) {
  // printf("scale factor: %f\n", scaleFactor_);
  convolver_->process(
      *processingBus->getChannel(0),
      *processingBus->getChannel(0),
      framesToProcess);
  for (int i = 0; i < framesToProcess; i++) {
    processingBus->getChannel(0)->getData()[i] *= scaleFactor_;
  }
  if (processingBus->getNumberOfChannels() > 1) {
    for (int channel = 1; channel < processingBus->getNumberOfChannels();
         ++channel) {
      processingBus->getChannel(channel)->copy(processingBus->getChannel(0));
    }
  }
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

  power = std::sqrtf(power / (numberOfChannels * length));
  if (power < MinPower) {
    power = MinPower;
  }
  scaleFactor_ = 1 / power;
  scaleFactor_ *= GainCalibration;
  scaleFactor_ *= GainCalibrationSampleRate / buffer_->getSampleRate();

  if (numberOfChannels == 4)
    scaleFactor_ *= 0.5;
}
} // namespace audioapi
