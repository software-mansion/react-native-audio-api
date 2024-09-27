#pragma once

#include <string>
#include "AudioParam.h"
#include "AudioScheduledSourceNode.h"

namespace audioapi {

using namespace facebook;
using namespace facebook::jni;

class OscillatorNode
    : public jni::HybridClass<OscillatorNode, AudioScheduledSourceNode> {
 public:
  static auto constexpr kJavaDescriptor =
      "Lcom/swmansion/audioapi/nodes/oscillator/OscillatorNode;";

  AudioParam *getFrequencyParam();
  AudioParam *getDetuneParam();
  std::string getWaveType();
  void setWaveType(const std::string &waveType);
};
} // namespace audioapi
