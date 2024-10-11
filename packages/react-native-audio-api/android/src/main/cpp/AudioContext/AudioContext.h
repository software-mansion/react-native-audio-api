#pragma once

#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <oboe/Oboe.h>

#include "AudioBuffer.h"
#include "AudioBufferSourceNode.h"
#include "AudioDestinationNode.h"
#include "AudioScheduledSourceNode.h"
#include "BiquadFilterNode.h"
#include "Constants.h"
#include "GainNode.h"
#include "OscillatorNode.h"
#include "StereoPannerNode.h"

namespace audioapi {

using namespace oboe;

class AudioContext: public AudioStreamDataCallback {
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

    DataCallbackResult onAudioReady(
            AudioStream *oboeStream,
            void *audioData,
            int32_t numFrames) override;

 private:
  std::shared_ptr<AudioDestinationNode> destination_;
    std::shared_ptr<AudioStream> mStream_;
  State state_ = State::RUNNING;
  int sampleRate_ = SAMPLE_RATE;
  double contextStartTime_;
};

} // namespace audioapi
