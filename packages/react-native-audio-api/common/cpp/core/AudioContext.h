#pragma once

#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "AudioBuffer.h"
#include "AudioBufferSourceNode.h"
#include "AudioDestinationNode.h"
#include "AudioScheduledSourceNode.h"
#include "BiquadFilterNode.h"
#include "Constants.h"
#include "GainNode.h"
#include "OscillatorNode.h"
#include "StereoPannerNode.h"

#ifdef ANDROID
#include "AudioPlayer.h"
#else
#include "IOSAudioPlayer.h"
#endif

namespace audioapi {

class AudioContext {
 public:
  AudioContext();
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
  static std::shared_ptr<AudioBuffer>
  createBuffer(int numberOfChannels, int length, int sampleRate);
  std::function<void(float *, int)> renderAudio();

 private:
  enum class State { SUSPENDED, RUNNING, CLOSED };

  static std::string toString(State state) {
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

 private:
  std::shared_ptr<AudioDestinationNode> destination_;
#ifdef ANDROID
  std::shared_ptr<AudioPlayer> audioPlayer_;
#else
  std::shared_ptr<IOSAudioPlayer> audioPlayer_;
#endif
  State state_ = State::RUNNING;
  int sampleRate_;
  double contextStartTime_;
};

} // namespace audioapi
