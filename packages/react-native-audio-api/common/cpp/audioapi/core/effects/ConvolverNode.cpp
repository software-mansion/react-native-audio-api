#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/effects/ConvolverNode.h>
#include <audioapi/core/sources/AudioBuffer.h>
#include <audioapi/core/utils/Constants.h>
#include <audioapi/dsp/AudioUtils.h>
#include <audioapi/dsp/FFT.h>
#include <audioapi/utils/AudioArray.h>
#include <iostream>

static int counter = 0;

namespace audioapi {
ConvolverNode::ConvolverNode(BaseAudioContext *context)
    : AudioNode(context),
      normalize_(true),
      buffer_(nullptr),
      convolver_(nullptr),
      internalBuffer_(nullptr) {
  channelCount_ = 1;
  audioBus_ = std::make_shared<AudioBus>(
      RENDER_QUANTUM_SIZE, channelCount_, context->getSampleRate());
  channelCountMode_ = ChannelCountMode::CLAMPED_MAX;
  gainCalibrationSampleRate_ = 44100.0f;
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
    convolver_->init(RENDER_QUANTUM_SIZE, audioArray, audioArray.getSize());
    internalBuffer_ = std::make_shared<AudioBus>(
        RENDER_QUANTUM_SIZE * 2, 1, buffer->getSampleRate());
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
  if (internalBufferIndex_ < framesToProcess) {
    convolver_->process(*processingBus->getChannel(0));

    internalBuffer_->copy(
        processingBus.get(), 0, internalBufferIndex_, RENDER_QUANTUM_SIZE);
    internalBufferIndex_ += RENDER_QUANTUM_SIZE;
  }
  processingBus->zero();
  processingBus->copy(internalBuffer_.get(), 0, 0, framesToProcess);
  int remainingFrames = internalBufferIndex_ - framesToProcess;
  if (remainingFrames > 0) {
    memmove(
        internalBuffer_->getChannel(0)->getData(),
        internalBuffer_->getChannel(0)->getData() + framesToProcess,
        remainingFrames * sizeof(float));
  }
  internalBufferIndex_ -= framesToProcess;

  dsp::multiplyByScalar(
      processingBus->getChannel(0)->getData(),
      scaleFactor_,
      processingBus->getChannel(0)->getData(),
      framesToProcess);
  return processingBus;
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
  if (power < minPower_) {
    power = minPower_;
  }
  scaleFactor_ = 1 / power;
  scaleFactor_ *= std::powf(10, gainCalibration_ * 0.05);
  scaleFactor_ *= gainCalibrationSampleRate_ / buffer_->getSampleRate();

  if (numberOfChannels == 4)
    scaleFactor_ *= 0.5;
}
} // namespace audioapi
