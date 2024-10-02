#pragma once

#include <fbjni/fbjni.h>
#include <jsi/jsi.h>
#include <react/jni/CxxModuleWrapper.h>
#include <react/jni/JMessageQueueThread.h>
#include <memory>
#include <string>
#include <utility>

#include "AudioBuffer.h"
#include "AudioBufferSourceNode.h"
#include "AudioContextHostObject.h"
#include "AudioContextWrapper.h"
#include "AudioDestinationNode.h"
#include "BiquadFilterNode.h"
#include "GainNode.h"
#include "OscillatorNode.h"
#include "StereoPannerNode.h"

namespace audioapi {

using namespace facebook;
using namespace facebook::jni;

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
  std::shared_ptr<AudioBuffer> createBuffer(int numberOfChannels, int length, int sampleRate);

 private:
    std::shared_ptr<AudioDestinationNode> destination_;
    std::string state_ = "running";
    int sampleRate_ = 44100;
    double contextStartTime_;
};

} // namespace audioapi
