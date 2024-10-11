#pragma once

#include <algorithm>
#include <vector>

#include "AudioNode.h"

namespace audioapi {

class AudioDestinationNode : public AudioNode {
 public:
  explicit AudioDestinationNode(AudioContext *context);

  void renderAudio(float *audioData, int32_t numFrames);

 protected:
  bool processAudio(float *audioData, int32_t numFrames) override;

 private:
  std::unique_ptr<float[]> mixingBuffer;
};
} // namespace audioapi
