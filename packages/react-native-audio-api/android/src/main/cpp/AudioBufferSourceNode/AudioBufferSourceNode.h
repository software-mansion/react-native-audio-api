#pragma once

#include "AudioBuffer.h"
#include "AudioScheduledSourceNode.h"

namespace audioapi {

// TODO implement AudioBufferSourceNode

class AudioBufferSourceNode : public AudioScheduledSourceNode {
 public:
  explicit AudioBufferSourceNode(AudioContext *context);

  bool processAudio(float *audioData, int32_t numFrames) override;
};
} // namespace audioapi
