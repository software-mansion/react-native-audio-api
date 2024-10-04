#pragma once

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "AudioBuffer.h"
#include "AudioBufferSourceNode.h"
#include "AudioDestinationNode.h"
#include "AudioScheduledSourceNode.h"
#include "BiquadFilterNode.h"
#include "GainNode.h"
#include "OscillatorNode.h"
#include "StereoPannerNode.h"

namespace audioapi {

class AudioContext {
 public:
  explicit AudioContext();
  std::string getState();
  int getSampleRate() const;
  double getCurrentTime() const;
  void close();

  std::shared_ptr<AudioDestinationNode> getDestination();
  std::shared_ptr<OscillatorNode> createOscillator();
  std::shared_ptr<GainNode> createGain();
  std::shared_ptr<StereoPannerNode> createStereoPanner();
  std::shared_ptr<BiquadFilterNode> createBiquadFilter();
  std::shared_ptr<AudioBufferSourceNode> createBufferSource();
  std::shared_ptr<AudioBuffer>
  createBuffer(int numberOfChannels, int length, int sampleRate);

 private:
  enum class State { RUNNING, CLOSED };

  static std::string toString(State state) {
    switch (state) {
      case State::RUNNING:
        return "running";
      case State::CLOSED:
        return "closed";
      default:
        throw std::invalid_argument("Unknown context state");
    }
  }

 private:
  std::shared_ptr<AudioDestinationNode> destination_;
  State state_ = State::RUNNING;
  int sampleRate_ = 44100;
  double contextStartTime_;
  std::vector<std::shared_ptr<AudioScheduledSourceNode>> sources_;
};

} // namespace audioapi
