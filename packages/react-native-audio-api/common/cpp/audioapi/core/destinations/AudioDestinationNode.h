#pragma once

#include <audioapi/core/AudioNode.h>
#include <audioapi/types/NodeOptions.h>
#include <audioapi/utils/AudioBuffer.hpp>

#include <atomic>
#include <cstddef>
#include <memory>

namespace audioapi {

class BaseAudioContext;

class AudioDestinationNode : public AudioNode {
 public:
  explicit AudioDestinationNode(const std::shared_ptr<BaseAudioContext> &context);

  /// @note Thread safe
  std::size_t getCurrentSampleFrame() const;

  /// @note Thread safe
  double getCurrentTime() const;

  /// @note Audio Thread only
  void renderAudio(const std::shared_ptr<DSPAudioBuffer> &audioData, int numFrames);

 protected:
  // DestinationNode is triggered by AudioContext using renderAudio
  // processNode function is not necessary and is never called.
  std::shared_ptr<DSPAudioBuffer> processNode(
      const std::shared_ptr<DSPAudioBuffer> &processingBuffer,
      int) final {
    return processingBuffer;
  };

 private:
  std::atomic<std::size_t> currentSampleFrame_;
};

} // namespace audioapi
