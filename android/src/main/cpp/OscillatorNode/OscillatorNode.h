#pragma once

#include <string>
#include "AudioNode.h"
#include "AudioParam.h"

namespace audioapi {

using namespace facebook;
using namespace facebook::jni;

class OscillatorNode : public jni::HybridClass<OscillatorNode, AudioNode> {
 public:
  static auto constexpr kJavaDescriptor =
      "Lcom/swmansion/audioapi/nodes/oscillator/OscillatorNode;";

  void start(double time);
  void stop(double time);
  AudioParam *getFrequencyParam();
  AudioParam *getDetuneParam();
  std::string getWaveType();
  void setWaveType(const std::string &waveType);
  void prepareForDeconstruction();
};
} // namespace audioapi
