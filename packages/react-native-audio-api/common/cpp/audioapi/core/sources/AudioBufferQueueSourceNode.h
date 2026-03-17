#pragma once

#include <audioapi/core/sources/AudioBufferBaseSourceNode.h>
#include <audioapi/libs/signalsmith-stretch/signalsmith-stretch.h>
#include <audioapi/utils/AudioBuffer.h>

#include <cstddef>
#include <list>
#include <memory>
#include <string>

namespace audioapi {

class AudioBuffer;
class AudioParam;
struct BaseAudioBufferSourceOptions;

class AudioBufferQueueSourceNode : public AudioBufferBaseSourceNode {
 public:
  explicit AudioBufferQueueSourceNode(
      const std::shared_ptr<BaseAudioContext> &context,
      const BaseAudioBufferSourceOptions &options);

  /// @note Audio Thread only
  void stop(double when) override;

  void start(double when) override;
  /// @note Audio Thread only
  void start(double when, double offset);
  /// @note Audio Thread only
  void pause();

  /// @note Audio Thread only
  void enqueueBuffer(
      const std::shared_ptr<AudioBuffer> &buffer,
      size_t bufferId,
      const std::shared_ptr<AudioBuffer> &tailBuffer);

  /// @note Audio Thread only
  void dequeueBuffer(size_t bufferId);
  /// @note Audio Thread only
  void clearBuffers();

  /// @note Audio Thread only
  void disable() override;

  /// @note Audio Thread only
  void setOnBufferEndedCallbackId(uint64_t callbackId);

  void unregisterOnBufferEndedCallback(uint64_t callbackId);

 protected:
  std::shared_ptr<AudioBuffer> processNode(
      const std::shared_ptr<AudioBuffer> &processingBuffer,
      int framesToProcess) override;

  double getCurrentPosition() const override;

  void sendOnBufferEndedEvent(size_t bufferId, bool isLastBufferInQueue);

 private:
  // User provided buffers
  std::list<std::pair<size_t, std::shared_ptr<AudioBuffer>>> buffers_{};

  bool isPaused_ = false;
  bool addExtraTailFrames_ = false;
  std::shared_ptr<AudioBuffer> tailBuffer_;

  double playedBuffersDuration_ = 0;

  uint64_t onBufferEndedCallbackId_ = 0; // 0 means no callback

  void processWithoutInterpolation(
      const std::shared_ptr<AudioBuffer> &processingBuffer,
      size_t startOffset,
      size_t offsetLength,
      float playbackRate) override;

  void processWithInterpolation(
      const std::shared_ptr<AudioBuffer> &processingBuffer,
      size_t startOffset,
      size_t offsetLength,
      float playbackRate) override;
};

} // namespace audioapi
