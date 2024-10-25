#pragma once

#include <algorithm>
#include <memory>
#include <vector>

#include "AudioNode.h"
#include "VectorMath.h"

namespace audioapi {

class AudioDestinationNode : public AudioNode {
 public:
  explicit AudioDestinationNode(AudioContext *context);

  void renderAudio(float *audioData, int32_t numFrames);

 protected:
  bool processAudio(float *audioData, int32_t numFrames) override;

 private:
  std::unique_ptr<float[]> mixingBuffer;

  void normalize(float *audioData, int32_t numFrames);
};
} // namespace audioapi
