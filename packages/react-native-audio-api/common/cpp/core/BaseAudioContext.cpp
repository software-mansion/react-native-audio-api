#ifdef ANDROID
#include "AudioPlayer.h"
#else
#include "IOSAudioPlayer.h"
#endif

#include "BaseAudioContext.h"

#include "GainNode.h"
#include "AudioBus.h"
#include "AudioArray.h"
#include "AudioBuffer.h"
#include "OscillatorNode.h"
#include "StereoPannerNode.h"
#include "BiquadFilterNode.h"
#include "AudioDestinationNode.h"
#include "AudioBufferSourceNode.h"

namespace audioapi {

BaseAudioContext::BaseAudioContext() {
#ifdef ANDROID
  audioPlayer_ = std::make_shared<AudioPlayer>(this->renderAudio());
#else
  audioPlayer_ = std::make_shared<IOSAudioPlayer>(this->renderAudio());
#endif

  sampleRate_ = audioPlayer_->getSampleRate();
  bufferSizeInFrames_ = audioPlayer_->getBufferSizeInFrames();

  destination_ = std::make_shared<AudioDestinationNode>(this);
  audioPlayer_->start();
}

std::string BaseAudioContext::getState() {
  return BaseAudioContext::toString(state_);
}

int BaseAudioContext::getSampleRate() const {
  return sampleRate_;
}

int BaseAudioContext::getBufferSizeInFrames() const {
  return bufferSizeInFrames_;
}

std::size_t BaseAudioContext::getCurrentSampleFrame() const {
  return destination_->getCurrentSampleFrame();
}

double BaseAudioContext::getCurrentTime() const {
  return destination_->getCurrentTime();
}

std::shared_ptr<AudioDestinationNode> BaseAudioContext::getDestination() {
  return destination_;
}

std::shared_ptr<OscillatorNode> BaseAudioContext::createOscillator() {
  return std::make_shared<OscillatorNode>(this);
}

std::shared_ptr<GainNode> BaseAudioContext::createGain() {
  return std::make_shared<GainNode>(this);
}

std::shared_ptr<StereoPannerNode> BaseAudioContext::createStereoPanner() {
  return std::make_shared<StereoPannerNode>(this);
}

std::shared_ptr<BiquadFilterNode> BaseAudioContext::createBiquadFilter() {
  return std::make_shared<BiquadFilterNode>(this);
}

std::shared_ptr<AudioBufferSourceNode> BaseAudioContext::createBufferSource() {
  return std::make_shared<AudioBufferSourceNode>(this);
}

std::shared_ptr<AudioBuffer> BaseAudioContext::createBuffer(
    int numberOfChannels,
    int length,
    int sampleRate) {
  return std::make_shared<AudioBuffer>(numberOfChannels, length, sampleRate);
}

std::function<void(AudioBus*, int)> BaseAudioContext::renderAudio() {
  if (state_ == State::CLOSED) {
    return [](AudioBus*, int) {};
  }

  return [this](AudioBus* data, int frames) {
    destination_->renderAudio(data, frames);
  };
}

std::string BaseAudioContext::toString(State state) {
  switch (state) {
    case State::SUSPENDED:
      return "suspended";
    case State::RUNNING:
      return "running";
    case State::CLOSED:
      return "closed";
    default:
      throw std::invalid_argument("Unknown context state");
  }
}
} // namespace audioapi
