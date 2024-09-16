#pragma once

#include "AudioBuffer.h"
#include "AudioNode.h"

namespace audioapi {

using namespace facebook;
using namespace facebook::jni;

class AudioBufferSourceNode
    : public jni::HybridClass<AudioBufferSourceNode, AudioNode> {
 public:
  static auto constexpr kJavaDescriptor =
      "Lcom/swmansion/audioapi/nodes/AudioBufferSourceNode;";

  void start(double time);
  void stop(double time);
  bool getLoop();
  void setLoop(bool loop);
  AudioBuffer *getBuffer();
  void setBuffer(const AudioBuffer *buffer);
};
} // namespace audioapi
