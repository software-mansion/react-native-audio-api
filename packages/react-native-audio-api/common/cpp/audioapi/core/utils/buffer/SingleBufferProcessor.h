#pragma once

#include "audioapi/core/utils/buffer/BufferProcessorBase.hpp"
namespace audioapi {

class SingleBufferProcessor : public BufferProcessorBase {
 public:
  SingleBufferProcessor(
      AudioBuffer *buffer,
      double *vReadIndex,
      bool loop,
      float rate,
      size_t startFrame,
      size_t endFrame)
      : BufferProcessorBase(buffer, vReadIndex, loop, rate, startFrame, endFrame) {}

  CursorState advance() override;
  void consume(size_t frames) override;
  size_t remainingInContiguousBlock() override;
  size_t currentIndex() override;
  const AudioBuffer *getBuffer() override;
  const AudioBuffer *getNextBuffer() override;
  bool atBoundary() override;
  bool shouldStop() override;
  void handleBoundary() override;
};

} // namespace audioapi