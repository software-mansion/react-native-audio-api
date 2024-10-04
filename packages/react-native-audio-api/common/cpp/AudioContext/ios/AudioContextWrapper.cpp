#ifndef ANDROID
#include "AudioContextWrapper.h"

namespace audioapi {
AudioContextWrapper::AudioContextWrapper()
    : audioContext_(std::make_shared<IOSAudioContext>()) {
  destination_ = std::make_shared<AudioDestinationNodeWrapper>(
      audioContext_->getDestination());
}

std::shared_ptr<AudioDestinationNodeWrapper>
AudioContextWrapper::getDestination() const {
  return destination_;
}

std::shared_ptr<OscillatorNodeWrapper> AudioContextWrapper::createOscillator() const {
  return std::make_shared<OscillatorNodeWrapper>(
      audioContext_->createOscillator());
}

std::shared_ptr<GainNodeWrapper> AudioContextWrapper::createGain() const{
  return std::make_shared<GainNodeWrapper>(audioContext_->createGain());
}

std::shared_ptr<StereoPannerNodeWrapper>
AudioContextWrapper::createStereoPanner() const {
  return std::make_shared<StereoPannerNodeWrapper>(
      audioContext_->createStereoPanner());
}

std::shared_ptr<BiquadFilterNodeWrapper>
AudioContextWrapper::createBiquadFilter() const {
  return std::make_shared<BiquadFilterNodeWrapper>(
      audioContext_->createBiquadFilter());
}

std::shared_ptr<AudioBufferSourceNodeWrapper>
AudioContextWrapper::createBufferSource() const {
  return std::make_shared<AudioBufferSourceNodeWrapper>(
      audioContext_->createBufferSource());
}

std::shared_ptr<AudioBufferWrapper> AudioContextWrapper::createBuffer(
    int numberOfChannels,
    int length,
    int sampleRate) const {
  return std::make_shared<AudioBufferWrapper>(
      audioContext_->createBuffer(numberOfChannels, length, sampleRate));
}

double AudioContextWrapper::getCurrentTime() const {
  return audioContext_->getCurrentTime();
}

std::string AudioContextWrapper::getState() const {
  return audioContext_->getState();
}

int AudioContextWrapper::getSampleRate() const {
  return audioContext_->getSampleRate();
}

void AudioContextWrapper::close() const {
  audioContext_->close();
}
} // namespace audioapi
#endif
