#pragma once

#include "AudioNode.h"
#include "AudioParam.h"

namespace audioapi {

using namespace facebook;
using namespace facebook::jni;

class StereoPannerNode : public jni::HybridClass<StereoPannerNode, AudioNode> {
 public:
  static auto constexpr kJavaDescriptor =
      "Lcom/swmansion/audioapi/nodes/StereoPannerNode;";

  AudioParam *getPanParam();
};

} // namespace audioapi
