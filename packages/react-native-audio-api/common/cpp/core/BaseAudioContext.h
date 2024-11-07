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
#include "ContextState.h"

#ifdef ANDROID
#include "AudioPlayer.h"
#else
#include "IOSAudioPlayer.h"
#endif

namespace audioapi {

class BaseAudioContext {
 public:
  BaseAudioContext();
  std::string getState();
  [[nodiscard]] int getSampleRate() const;
  [[nodiscard]] double getCurrentTime() const;

  std::shared_ptr<AudioDestinationNode> getDestination();
  std::shared_ptr<OscillatorNode> createOscillator();
  std::shared_ptr<GainNode> createGain();
  std::shared_ptr<StereoPannerNode> createStereoPanner();
  std::shared_ptr<BiquadFilterNode> createBiquadFilter();
  std::shared_ptr<AudioBufferSourceNode> createBufferSource();
  static std::shared_ptr<AudioBuffer>
  createBuffer(int numberOfChannels, int length, int sampleRate);
  std::function<void(float *, int)> renderAudio();

 protected:
  std::shared_ptr<AudioDestinationNode> destination_;
#ifdef ANDROID
  std::shared_ptr<AudioPlayer> audioPlayer_;
#else
  std::shared_ptr<IOSAudioPlayer> audioPlayer_;
#endif
  ContextState state_ = ContextState::RUNNING;
  int sampleRate_;
  double contextStartTime_;

private:
    static std::string toString(ContextState state) {
        switch (state) {
            case ContextState::SUSPENDED:
                return "suspended";
            case ContextState::RUNNING:
                return "running";
            case ContextState::CLOSED:
                return "closed";
            default:
                throw std::invalid_argument("Unknown context state");
        }
    }
};

} // namespace audioapi
