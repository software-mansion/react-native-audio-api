#ifndef ANDROID
#include "AudioContextWrapper.h"

namespace audioapi {
AudioContextWrapper::AudioContextWrapper() : audiocontext_(std::make_shared<IOSAudioContext>()) {
    auto destinationNode = audiocontext_->getDestination();
    destinationNode_ = std::make_shared<AudioDestinationNodeWrapper>(destinationNode);
    sampleRate_ = audiocontext_->getSampleRate();
}

std::shared_ptr<OscillatorNodeWrapper> AudioContextWrapper::createOscillator() {
  return std::make_shared<OscillatorNodeWrapper>(audiocontext_);
}

std::shared_ptr<AudioDestinationNodeWrapper>
AudioContextWrapper::getDestination() const {
    return destinationNode_;
}

std::shared_ptr<GainNodeWrapper> AudioContextWrapper::createGain() {
    auto gainNode = audiocontext_->createGain();
  return std::make_shared<GainNodeWrapper>(gainNode);
}

std::shared_ptr<StereoPannerNodeWrapper>
AudioContextWrapper::createStereoPanner() {
    auto stereoPannerNode = audiocontext_->createStereoPanner();
  return std::make_shared<StereoPannerNodeWrapper>(stereoPannerNode);
}

std::shared_ptr<BiquadFilterNodeWrapper>
AudioContextWrapper::createBiquadFilter() {
    auto biquadFilterNode = audiocontext_->createBiquadFilter();
  return std::make_shared<BiquadFilterNodeWrapper>(biquadFilterNode);
}

std::shared_ptr<AudioBufferSourceNodeWrapper>
AudioContextWrapper::createBufferSource() {
    auto bufferSourceNode = audiocontext_->createBufferSource();
  return std::make_shared<AudioBufferSourceNodeWrapper>(bufferSourceNode);
}

std::shared_ptr<AudioBufferWrapper> AudioContextWrapper::createBuffer(
    int sampleRate,
    int length,
    int numberOfChannels) {
    auto audioBuffer = audiocontext_->createBuffer(sampleRate,length,numberOfChannels);
  return std::make_shared<AudioBufferWrapper>(
      audioBuffer);
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
