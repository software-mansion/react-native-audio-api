#pragma once

#include <audioapi/core/AudioNode.h>

#include <algorithm>
#include <memory>
#include <vector>
#include <cstddef>

namespace audioapi {

class AudioBus;
class BaseAudioContext;

class AudioDestinationNode : public AudioNode {
 public:
  explicit AudioDestinationNode(BaseAudioContext *context);

  std::size_t getCurrentSampleFrame() const;
  double getCurrentTime() const;

  void renderAudio(const std::shared_ptr<AudioBus>& audioData, int numFrames);

 protected:
  // DestinationNode is triggered by AudioContext using renderAudio
  // processNode function is not necessary and is never called.
  std::shared_ptr<AudioBus> processNode(const std::shared_ptr<AudioBus>& processingBus, int) final { return processingBus; };

 private:
  std::size_t currentSampleFrame_;
};

} // namespace audioapi
