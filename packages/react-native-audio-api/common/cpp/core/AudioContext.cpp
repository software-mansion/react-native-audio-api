#include "AudioContext.h"

namespace audioapi {

AudioContext::AudioContext() {
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

std::string AudioContext::getState() {
  return AudioContext::toString(state_);
}

int AudioContext::getSampleRate() const {
  return sampleRate_;
}

double AudioContext::getCurrentTime() const {
  auto now = std::chrono::high_resolution_clock ::now();
  auto currentTime =
      static_cast<double>(std::chrono::duration_cast<std::chrono::nanoseconds>(
                              now.time_since_epoch())
                              .count());
  return (currentTime - contextStartTime_) / 1e9;
}

void AudioContext::close() {
  state_ = State::CLOSED;

  if (audioPlayer_) {
    audioPlayer_->stop();
  }

  audioPlayer_.reset();
  destination_.reset();
}

std::shared_ptr<AudioDestinationNode> AudioContext::getDestination() {
  return destination_;
}

std::shared_ptr<OscillatorNode> AudioContext::createOscillator() {
  return std::make_shared<OscillatorNode>(this);
}

std::shared_ptr<GainNode> AudioContext::createGain() {
  return std::make_shared<GainNode>(this);
}

std::shared_ptr<StereoPannerNode> AudioContext::createStereoPanner() {
  return std::make_shared<StereoPannerNode>(this);
}

std::shared_ptr<BiquadFilterNode> AudioContext::createBiquadFilter() {
  return std::make_shared<BiquadFilterNode>(this);
}

std::shared_ptr<AudioBufferSourceNode> AudioContext::createBufferSource() {
  return std::make_shared<AudioBufferSourceNode>(this);
}

std::shared_ptr<AudioBuffer>
AudioContext::createBuffer(int numberOfChannels, int length, int sampleRate) {
  return std::make_shared<AudioBuffer>(numberOfChannels, length, sampleRate);
}

std::function<void(float*, int)> AudioContext::renderAudio() {
  if (state_ == State::CLOSED) {
    return [](float*, int) {};
  }

  return [this](float* data, int frames) {
    destination_->renderAudio(data, frames);
  };
}
} // namespace audioapi
