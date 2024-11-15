#pragma once

#include <vector>
#include <memory>
#include <algorithm>

#include "AudioNode.h"

namespace audioapi {

class AudioBus;
class BaseAudioContext;

class AudioDestinationNode : public AudioNode {
 public:
  explicit AudioDestinationNode(BaseAudioContext *context);

  void renderAudio(AudioBus* audioData, int32_t numFrames);

  unsigned getCurrentSampleFrame() const;
  double getCurrentTime() const;

 protected:

 private:
  unsigned currentSampleFrame_;

  // DestinationNode is triggered by AudioContext using renderAudio
  // processNode function is not necessary and is never called.
  void processNode(int) final { };
};

} // namespace audioapi
