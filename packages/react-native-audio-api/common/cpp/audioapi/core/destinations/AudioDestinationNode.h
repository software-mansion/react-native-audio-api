#pragma once

#include <audioapi/core/AudioNode.h>
#include <audioapi/types/NodeOptions.h>

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <memory>
#include <vector>

namespace audioapi {

class AudioBuffer;
class BaseAudioContext;

class AudioDestinationNode : public AudioNode {
 public:
  explicit AudioDestinationNode(const std::shared_ptr<BaseAudioContext> &context);

  std::size_t getCurrentSampleFrame() const;
  double getCurrentTime() const;

  void renderAudio(const std::shared_ptr<AudioBuffer> &audioData, int numFrames);

 protected:
  // DestinationNode is triggered by AudioContext using renderAudio
  // processNode function is not necessary and is never called.
  std::shared_ptr<AudioBuffer> processNode(
      const std::shared_ptr<AudioBuffer> &processingBuffer,
      int) final {
    return processingBuffer;
  };

 private:
  std::atomic<std::size_t> currentSampleFrame_;
};

} // namespace audioapi
