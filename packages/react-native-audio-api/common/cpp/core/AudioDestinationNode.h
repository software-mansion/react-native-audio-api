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

  std::size_t getCurrentSampleFrame() const;
  double getCurrentTime() const;

 protected:
  // DestinationNode is triggered by AudioContext using renderAudio
  // processNode function is not necessary and is never called.
  void processNode(AudioBus*, int) final { };

 private:
  std::size_t currentSampleFrame_;
};

} // namespace audioapi
