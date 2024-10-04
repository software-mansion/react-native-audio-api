#pragma once

#include <algorithm>

#include "AudioNode.h"

namespace audioapi {

class AudioDestinationNode : public AudioNode {
 public:
  explicit AudioDestinationNode(AudioContext *context);

 protected:
  void process(
      AudioStream *oboeStream,
      void *audioData,
      int32_t numFrames,
      int channelCount) override;

 private:
  static void normalize(void *audioData, int32_t numFrames, int channelCount);
};

} // namespace audioapi
