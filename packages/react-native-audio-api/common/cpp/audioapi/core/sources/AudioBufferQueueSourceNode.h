#pragma once

#include <audioapi/core/sources/AudioBuffer.h>
#include <audioapi/core/sources/AudioBufferBaseSourceNode.h>
#include <audioapi/libs/signalsmith-stretch/signalsmith-stretch.h>

#include <algorithm>
#include <cstddef>
#include <memory>
#include <queue>
#include <string>

namespace audioapi {

class AudioBus;
class AudioParam;

class AudioBufferQueueSourceNode : public AudioBufferBaseSourceNode {
 public:
  explicit AudioBufferQueueSourceNode(
      std::shared_ptr<BaseAudioContext> context,
      bool pitchCorrection);
  ~AudioBufferQueueSourceNode() override;

  void stop(double when) override;

  void start(double when) override;
  void start(double when, double offset);
  void pause();

  std::string enqueueBuffer(const std::shared_ptr<AudioBuffer> &buffer);
  void dequeueBuffer(size_t bufferId);
  void clearBuffers();
  void disable() override;

 protected:
  std::shared_ptr<AudioBus> processNode(
      const std::shared_ptr<AudioBus> &processingBus,
      int framesToProcess) override;
  double getCurrentPosition() const override;

 private:
  // User provided buffers
  std::queue<std::pair<size_t, std::shared_ptr<AudioBuffer>>> buffers_;
  size_t bufferId_ = 0;

  bool isPaused_ = false;
  bool addExtraTailFrames_ = false;
  std::shared_ptr<AudioBuffer> tailBuffer_;

  double playedBuffersDuration_ = 0;

  void processWithoutInterpolation(
      const std::shared_ptr<AudioBus> &processingBus,
      size_t startOffset,
      size_t offsetLength,
      float playbackRate) override;

  void processWithInterpolation(
      const std::shared_ptr<AudioBus> &processingBus,
      size_t startOffset,
      size_t offsetLength,
      float playbackRate) override;
};

} // namespace audioapi
