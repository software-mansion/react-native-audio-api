#include "BaseAudioContext.h"

namespace audioapi {

BaseAudioContext::BaseAudioContext() {
#ifdef ANDROID
  audioPlayer_ = std::make_shared<AudioPlayer>(this->renderAudio());
#else
  audioPlayer_ = std::make_shared<IOSAudioPlayer>(this->renderAudio());
#endif
  destination_ = std::make_shared<AudioDestinationNode>(this);

  sampleRate_ = audioPlayer_->getSampleRate();

  auto now = std::chrono::high_resolution_clock ::now();
  contextStartTime_ =
      static_cast<double>(std::chrono::duration_cast<std::chrono::nanoseconds>(
                              now.time_since_epoch())
                              .count());

  audioPlayer_->start();
}

std::string BaseAudioContext::getState() {
  return BaseAudioContext::toString(state_);
}

int BaseAudioContext::getSampleRate() const {
  return sampleRate_;
}

double BaseAudioContext::getCurrentTime() const {
  auto now = std::chrono::high_resolution_clock ::now();
  auto currentTime =
      static_cast<double>(std::chrono::duration_cast<std::chrono::nanoseconds>(
                              now.time_since_epoch())
                              .count());
  return (currentTime - contextStartTime_) / 1e9;
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

std::function<void(float *, int)> BaseAudioContext::renderAudio() {
  if (state_ == ContextState::CLOSED) {
    return [](float *, int) {};
  }

  return [this](float *data, int frames) {
    destination_->renderAudio(data, frames);
  };
}
} // namespace audioapi
