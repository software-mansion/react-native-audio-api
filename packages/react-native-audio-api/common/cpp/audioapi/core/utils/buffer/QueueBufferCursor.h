#pragma once

#include <audioapi/core/utils/buffer/PlaybackCursor.h>
#include <cstddef>
#include <functional>
#include <list>
#include <memory>
#include <utility>

namespace audioapi {

class QueueBufferCursor {
 public:
  using OnBufferConsumed = std::function<void(
      size_t bufferId,
      std::shared_ptr<AudioBuffer> buffer,
      bool isLastInQueue,
      bool fireBufferEndedEvent)>;

  QueueBufferCursor(
      std::list<std::pair<size_t, std::shared_ptr<AudioBuffer>>> *buffers,
      double *vReadIndex,
      float rate,
      OnBufferConsumed onBufferConsumed = {})
      : buffers_(buffers),
        vReadIndex_(vReadIndex),
        rate_(rate),
        onBufferConsumed_(std::move(onBufferConsumed)) {}

  /// Arm an in-place tail buffer. When the main queue would drain during
  /// processing, handleBoundary() appends this tail instead of stopping.
  void setPendingTail(std::shared_ptr<AudioBuffer> tailBuffer) {
    pendingTailBuffer_ = std::move(tailBuffer);
  }

  [[nodiscard]] bool didConsumeTail() const {
    return tailConsumed_;
  }

  CursorState advance();
  void consume(size_t frames = 1);

  [[nodiscard]] const AudioBuffer *getBuffer() const;
  [[nodiscard]] const AudioBuffer *getNextBuffer() const;
  [[nodiscard]] size_t remainingInContiguousBlock() const;
  [[nodiscard]] size_t currentIndex() const;

  void handleBoundary();
  [[nodiscard]] bool atBoundary() const;
  [[nodiscard]] bool shouldStop() const;

  [[nodiscard]] double getVirtualReadIndex() const;

 private:
  std::list<std::pair<size_t, std::shared_ptr<AudioBuffer>>> *buffers_;
  double *vReadIndex_;
  float rate_;
  OnBufferConsumed onBufferConsumed_;

  std::shared_ptr<AudioBuffer> pendingTailBuffer_ = nullptr;
  bool tailConsumed_ = false;
};

static_assert(
    PlaybackCursor<QueueBufferCursor>,
    "QueueBufferCursor must satisfy PlaybackCursor concept");
}; // namespace audioapi
