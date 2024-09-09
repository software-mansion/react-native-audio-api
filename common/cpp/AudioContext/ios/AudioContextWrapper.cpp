#ifndef ANDROID
#include "AudioContextWrapper.h"

namespace audioapi {
AudioContextWrapper::AudioContextWrapper()
    : audiocontext_(std::make_shared<IOSAudioContext>()) {
  sampleRate_ = audiocontext_->getSampleRate();
  destinationNode_ = std::make_shared<AudioDestinationNodeWrapper>(
      audiocontext_->getDestination());
}

std::shared_ptr<AudioDestinationNodeWrapper>
AudioContextWrapper::getDestination() const {
  return destinationNode_;
}

std::shared_ptr<OscillatorNodeWrapper> AudioContextWrapper::createOscillator() {
  return std::make_shared<OscillatorNodeWrapper>(
      audiocontext_->createOscillator());
}

std::shared_ptr<GainNodeWrapper> AudioContextWrapper::createGain() {
  return std::make_shared<GainNodeWrapper>(audiocontext_->createGain());
}

std::shared_ptr<StereoPannerNodeWrapper>
AudioContextWrapper::createStereoPanner() {
  return std::make_shared<StereoPannerNodeWrapper>(
      audiocontext_->createStereoPanner());
}

std::shared_ptr<BiquadFilterNodeWrapper>
AudioContextWrapper::createBiquadFilter() {
  return std::make_shared<BiquadFilterNodeWrapper>(
      audiocontext_->createBiquadFilter());
}

std::shared_ptr<AudioBufferSourceNodeWrapper>
AudioContextWrapper::createBufferSource() {
  return std::make_shared<AudioBufferSourceNodeWrapper>(
      audiocontext_->createBufferSource());
}

std::shared_ptr<AudioBufferWrapper> AudioContextWrapper::createBuffer(
    int sampleRate,
    int length,
    int numberOfChannels) {
  return std::make_shared<AudioBufferWrapper>();
}

double AudioContextWrapper::getCurrentTime() {
  return audiocontext_->getCurrentTime();
}

std::string AudioContextWrapper::getState() {
  return audiocontext_->getState();
}

int AudioContextWrapper::getSampleRate() const {
  return sampleRate_;
}

void AudioContextWrapper::close() {
  audiocontext_->close();
}
} // namespace audioapi
#endif
