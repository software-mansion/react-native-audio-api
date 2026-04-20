#pragma once

#include <audioapi/core/utils/buffer/BufferProcessingDirection.hpp>
#include <audioapi/core/utils/buffer/PlaybackCursor.h>
namespace audioapi {

class SingleBufferCursor {
 public:
  SingleBufferCursor(
      const AudioBuffer *buffer,
      double *vReadIndex,
      bool loop,
      float rate,
      size_t startFrame,
      size_t endFrame)
      : buffer_(buffer),
        vReadIndex_(vReadIndex),
        loop_(loop),
        rate_(rate),
        startFrame_(startFrame),
        endFrame_(endFrame),
        direction_(
            rate_ >= 0 ? BufferProcessingDirection::FORWARD : BufferProcessingDirection::REVERSE) {}

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
  const AudioBuffer *buffer_;
  double *vReadIndex_;
  bool loop_;
  float rate_;
  size_t startFrame_, endFrame_;
  BufferProcessingDirection direction_;
};

static_assert(
    PlaybackCursor<SingleBufferCursor>,
    "SingleBufferCursor must satisfy PlaybackCursor concept");
}; // namespace audioapi