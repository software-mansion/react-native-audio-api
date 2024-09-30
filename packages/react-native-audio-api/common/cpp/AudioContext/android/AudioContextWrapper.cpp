#ifdef ANDROID
#include "AudioContextWrapper.h"

namespace audioapi {

AudioContextWrapper::AudioContextWrapper(AudioContext *audiocontext)
    : audiocontext_(audiocontext) {
  auto destination = audiocontext_->getDestination();
  destinationNode_ = std::make_shared<AudioDestinationNodeWrapper>(destination);
  sampleRate_ = audiocontext_->getSampleRate();
}

std::shared_ptr<OscillatorNodeWrapper> AudioContextWrapper::createOscillator() {
  auto oscillator = audiocontext_->createOscillator();
  return std::make_shared<OscillatorNodeWrapper>(oscillator);
}

std::shared_ptr<AudioDestinationNodeWrapper>
AudioContextWrapper::getDestination() const {
  return destinationNode_;
}

std::shared_ptr<GainNodeWrapper> AudioContextWrapper::createGain() {
  auto gain = audiocontext_->createGain();
  return std::make_shared<GainNodeWrapper>(gain);
}

std::shared_ptr<StereoPannerNodeWrapper>
AudioContextWrapper::createStereoPanner() {
  auto panner = audiocontext_->createStereoPanner();
  return std::make_shared<StereoPannerNodeWrapper>(panner);
}

std::shared_ptr<BiquadFilterNodeWrapper>
AudioContextWrapper::createBiquadFilter() {
  auto filter = audiocontext_->createBiquadFilter();
  return std::make_shared<BiquadFilterNodeWrapper>(filter);
}

std::shared_ptr<AudioBufferSourceNodeWrapper>
AudioContextWrapper::createBufferSource() {
  auto bufferSource = audiocontext_->createBufferSource();
  return std::make_shared<AudioBufferSourceNodeWrapper>(bufferSource);
}

std::shared_ptr<AudioBufferWrapper> AudioContextWrapper::createBuffer(
    int numberOfChannels,
    int length,
    int sampleRate) {
  auto buffer =
      audiocontext_->createBuffer(numberOfChannels, length, sampleRate);
  return std::make_shared<AudioBufferWrapper>(buffer);
}

std::string AudioContextWrapper::getState() {
  return audiocontext_->getState();
}

int AudioContextWrapper::getSampleRate() const {
  return sampleRate_;
}

double AudioContextWrapper::getCurrentTime() {
  return audiocontext_->getCurrentTime();
}

void AudioContextWrapper::close() {
  audiocontext_->close();
}
} // namespace audioapi
#endif
