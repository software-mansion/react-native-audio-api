#ifdef ANDROID
#include "AudioContextWrapper.h"

namespace audioapi {

AudioContextWrapper::AudioContextWrapper(
    const std::shared_ptr<AudioContext> &audiocontext)
    : audioContext_(audiocontext) {
  auto destination = audioContext_->getDestination();
  destination_ = std::make_shared<AudioDestinationNodeWrapper>(destination);
}

std::shared_ptr<AudioDestinationNodeWrapper>
AudioContextWrapper::getDestination() const {
  return destination_;
}

std::shared_ptr<OscillatorNodeWrapper> AudioContextWrapper::createOscillator()
    const {
  auto oscillator = audioContext_->createOscillator();
  return std::make_shared<OscillatorNodeWrapper>(oscillator);
}

std::shared_ptr<GainNodeWrapper> AudioContextWrapper::createGain() const {
  auto gain = audioContext_->createGain();
  return std::make_shared<GainNodeWrapper>(gain);
}

std::shared_ptr<StereoPannerNodeWrapper>
AudioContextWrapper::createStereoPanner() const {
  auto panner = audioContext_->createStereoPanner();
  return std::make_shared<StereoPannerNodeWrapper>(panner);
}

std::shared_ptr<BiquadFilterNodeWrapper>
AudioContextWrapper::createBiquadFilter() const {
  auto filter = audioContext_->createBiquadFilter();
  return std::make_shared<BiquadFilterNodeWrapper>(filter);
}

std::shared_ptr<AudioBufferSourceNodeWrapper>
AudioContextWrapper::createBufferSource() const {
  auto bufferSource = audioContext_->createBufferSource();
  return std::make_shared<AudioBufferSourceNodeWrapper>(bufferSource);
}

std::shared_ptr<AudioBufferWrapper> AudioContextWrapper::createBuffer(
    int numberOfChannels,
    int length,
    int sampleRate) const {
  auto buffer =
      audioContext_->createBuffer(numberOfChannels, length, sampleRate);
  return std::make_shared<AudioBufferWrapper>(buffer);
}

std::string AudioContextWrapper::getState() const {
  return audioContext_->getState();
}

int AudioContextWrapper::getSampleRate() const {
  return audioContext_->getSampleRate();
}

double AudioContextWrapper::getCurrentTime() const {
  return audioContext_->getCurrentTime();
}

void AudioContextWrapper::close() const {
  audioContext_->close();
}
} // namespace audioapi
#endif
