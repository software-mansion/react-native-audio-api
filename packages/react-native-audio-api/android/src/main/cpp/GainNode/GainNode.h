#pragma once

#include "AudioNode.h"
#include "AudioParam.h"

namespace audioapi {

using namespace facebook;
using namespace facebook::jni;

class GainNode : public jni::HybridClass<GainNode, AudioNode> {
 public:
  static auto constexpr kJavaDescriptor =
      "Lcom/swmansion/audioapi/nodes/GainNode;";

  AudioParam *getGainParam();
};

} // namespace audioapi
