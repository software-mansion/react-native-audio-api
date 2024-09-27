#pragma once

#include "AudioNode.h"

namespace audioapi {

using namespace facebook;
using namespace facebook::jni;

class AudioScheduledSourceNode
    : public jni::HybridClass<AudioScheduledSourceNode, AudioNode> {
 public:
  static auto constexpr kJavaDescriptor =
      "Lcom/swmansion/audioapi/nodes/AudioScheduledSourceNode;";

  void start(double time);
  void stop(double time);
  void prepareForDeconstruction();
};

} // namespace audioapi
