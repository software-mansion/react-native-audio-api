#include "AudioContext.h"

namespace audioapi {

using namespace facebook::jni;

AudioContext::AudioContext() {
    destination_ = std::make_shared<AudioDestinationNode>();
    auto now = std::chrono::high_resolution_clock ::now();
    contextStartTime_ = (double) std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
}

std::string AudioContext::getState() {
    return state_;
}

int AudioContext::getSampleRate() const {
    return sampleRate_;
}

double AudioContext::getCurrentTime() const {
    auto now = std::chrono::high_resolution_clock ::now();
    auto currentTime = (double) std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
    return (currentTime - contextStartTime_) / 1e9;
}

void AudioContext::close() {
    //TODO implement
}

std::shared_ptr<AudioDestinationNode> AudioContext::getDestination() {
  return destination_;
}

std::shared_ptr<OscillatorNode> AudioContext::createOscillator() {
  return std::make_shared<OscillatorNode>();
}

std::shared_ptr<GainNode> AudioContext::createGain() {
  return std::make_shared<GainNode>();
}

std::shared_ptr<StereoPannerNode> AudioContext::createStereoPanner() {
  return std::make_shared<StereoPannerNode>();
}

std::shared_ptr<BiquadFilterNode> AudioContext::createBiquadFilter() {
  return std::make_shared<BiquadFilterNode>();
}

std::shared_ptr<AudioBufferSourceNode> AudioContext::createBufferSource() {
  return std::make_shared<AudioBufferSourceNode>();
}

std::shared_ptr<AudioBuffer>
AudioContext::createBuffer(int numberOfChannels, int length, int sampleRate) {
  return std::make_shared<AudioBuffer>(numberOfChannels, length, sampleRate);
}
} // namespace audioapi
