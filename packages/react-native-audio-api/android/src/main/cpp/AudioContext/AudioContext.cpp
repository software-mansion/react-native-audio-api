#include "AudioContext.h"

namespace audioapi {

AudioContext::AudioContext() {
  destination_ = std::make_shared<AudioDestinationNode>(this);
  auto now = std::chrono::high_resolution_clock ::now();
  contextStartTime_ =
      static_cast<double>(std::chrono::duration_cast<std::chrono::nanoseconds>(
                              now.time_since_epoch())
                              .count());
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
  std::for_each(sources_.begin(), sources_.end(), [](auto &source) {
    source->cleanup();
  });
  sources_.clear();
}

std::shared_ptr<AudioDestinationNode> AudioContext::getDestination() {
  return destination_;
}

std::shared_ptr<OscillatorNode> AudioContext::createOscillator() {
  auto oscillator = std::make_shared<OscillatorNode>(this);
  sources_.push_back(oscillator);
  return oscillator;
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
  auto bufferSource = std::make_shared<AudioBufferSourceNode>(this);
  sources_.push_back(bufferSource);
  return bufferSource;
}

std::shared_ptr<AudioBuffer>
AudioContext::createBuffer(int numberOfChannels, int length, int sampleRate) {
  return std::make_shared<AudioBuffer>(numberOfChannels, length, sampleRate);
}
} // namespace audioapi
