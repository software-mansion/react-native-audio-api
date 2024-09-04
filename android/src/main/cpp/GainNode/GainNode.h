#pragma once

#include "AudioNode.h"
#include "AudioParam.h"

namespace audiocontext {

using namespace facebook;
using namespace facebook::jni;

class GainNode : public jni::HybridClass<GainNode, AudioNode> {
 public:
  static auto constexpr kJavaDescriptor = "Lcom/audiocontext/nodes/GainNode;";

  AudioParam *getGainParam();
};

} // namespace audiocontext
