#pragma once

#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/sources/AudioBufferBaseSourceNode.h>
#include <audioapi/utils/AudioBuffer.hpp>

#include <audioapi/core/utils/buffer/QueueBufferProcessor.h>
#include <cstddef>
#include <list>
#include <memory>

namespace audioapi {

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

  void resume(double when);

  /// @note Audio Thread only
  void enqueueBuffer(const std::shared_ptr<AudioBuffer> &buffer, size_t bufferId);

  /// @brief Declares that no further buffers will be enqueued.
  /// Once the queue empties after this call, playback treats that as natural EOF
  /// (WSOLA drain when pitch correction is on, then finish). Without this call,
  /// an empty queue is only an underrun and the node stays alive.
  /// @note Audio Thread only
  void endOfStream();

  /// @note Audio Thread only
  void dequeueBuffer(size_t bufferId);
  /// @note Audio Thread only
  void clearBuffers();

  /// @note Audio Thread only
  void disable() override;

  void assignOnBufferEndedCallbackId(uint64_t callbackId);

  /// @brief Set the channel count of the node. Channel count is set only once when the first buffer is enqueued.
  /// @param channelCount The channel count to set.
  /// @note Audio Thread only
  void setChannelCount(int channelCount);

 protected:
  double getCurrentPosition() const override;

  void sendOnBufferEndedEvent(size_t bufferId, bool isLastBufferInQueue);

  bool isEmpty() const final;

  void runBufferProcessor(
      const std::shared_ptr<DSPAudioBuffer> &processingBuffer,
      size_t startOffset,
      size_t offsetLength,
      float playbackRate,
      bool interpolate) final;

 private:
  // User provided buffers
  std::list<std::pair<size_t, std::shared_ptr<AudioBuffer>>> buffers_;

  bool isPaused_ = false;
  /// Set to true by @ref endOfStream().
  /// When true, an empty queue indicates EOF (drain and stop);
  /// when false, an empty queue is treated as a temporary underrun (keep waiting).
  bool endOfStream_ = false;

  double playedBuffersDuration_ = 0;

  EventCaller<AudioEvent::BUFFER_ENDED> onBufferEndedEvent_;

  std::unique_ptr<QueueBufferProcessor> processor_;

  void armNaturalEofIfNeeded();
};

} // namespace audioapi
