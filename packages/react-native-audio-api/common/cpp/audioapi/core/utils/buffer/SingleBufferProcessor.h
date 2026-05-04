#pragma once

#include <audioapi/core/utils/buffer/BufferProcessingDirection.hpp>
#include <audioapi/core/utils/buffer/BufferProcessorBase.hpp>

#include <cstddef>

namespace audioapi {

class SingleBufferProcessor : public BufferProcessorBase {
 public:
  SingleBufferProcessor(
      const AudioBuffer *buffer,
      double position,
      bool loop,
      float rate,
      double startFrame,
      double endFrame)
      : BufferProcessorBase(position, rate),
        buffer_(buffer),
        loop_(loop),
        startFrame_(startFrame),
        endFrame_(endFrame),
        direction_(
            rate >= 0 ? BufferProcessingDirection::FORWARD : BufferProcessingDirection::REVERSE) {}

  [[nodiscard]] bool atBoundary() const override;
  [[nodiscard]] bool shouldStop() const override;

 protected:
  CursorState advance() override;
  void consume(size_t frames) override;
  [[nodiscard]] size_t remainingInContiguousBlock() const override;
  [[nodiscard]] size_t currentIndex() const override;
  [[nodiscard]] const AudioBuffer *getBuffer() const override;
  [[nodiscard]] const AudioBuffer *getNextBuffer() const override;
  void handleBoundary() override;
  [[nodiscard]] bool isReverse() const override {
    return direction_ == BufferProcessingDirection::REVERSE;
  }

 private:
  const AudioBuffer *buffer_;
  bool loop_;
  double startFrame_;
  double endFrame_;
  BufferProcessingDirection direction_;
};

} // namespace audioapi
