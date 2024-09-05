#ifndef ANDROID
#include "AudioContextWrapper.h"

namespace audioapi {
std::shared_ptr<OscillatorNodeWrapper> AudioContextWrapper::createOscillator() {
  return std::make_shared<OscillatorNodeWrapper>(audiocontext_);
}

std::shared_ptr<AudioDestinationNodeWrapper>
AudioContextWrapper::getDestination() const {
  return std::make_shared<AudioDestinationNodeWrapper>(audiocontext_);
}

std::shared_ptr<GainNodeWrapper> AudioContextWrapper::createGain() {
  return std::make_shared<GainNodeWrapper>(audiocontext_);
}

std::shared_ptr<StereoPannerNodeWrapper>
AudioContextWrapper::createStereoPanner() {
  return std::make_shared<StereoPannerNodeWrapper>(audiocontext_);
}

std::shared_ptr<BiquadFilterNodeWrapper>
AudioContextWrapper::createBiquadFilter() {
  return std::make_shared<BiquadFilterNodeWrapper>(audiocontext_);
}

std::shared_ptr<AudioBufferSourceNodeWrapper>
AudioContextWrapper::createBufferSource() {
  return std::make_shared<AudioBufferSourceNodeWrapper>();
}

std::shared_ptr<AudioBufferWrapper> AudioContextWrapper::createBuffer(
    int sampleRate,
    int length,
    int numberOfChannels) {
  return std::make_shared<AudioBufferWrapper>(
      sampleRate, length, numberOfChannels);
}

double AudioContextWrapper::getCurrentTime() {
  return audiocontext_->getCurrentTime();
}

std::string AudioContextWrapper::getState() {
  return audiocontext_->getState();
}

int AudioContextWrapper::getSampleRate() const {
  return audiocontext_->getSampleRate();
}

void AudioContextWrapper::close() {
  audiocontext_->close();
}
} // namespace audioapi
#endif
