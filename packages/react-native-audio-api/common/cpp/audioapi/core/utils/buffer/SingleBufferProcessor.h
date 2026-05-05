#pragma once

#include <audioapi/core/utils/buffer/BufferProcessingDirection.h>
#include <audioapi/core/utils/buffer/BufferProcessorBase.h>

#include <cstddef>
#include "audioapi/utils/AudioBuffer.hpp"

namespace audioapi {

class SingleBufferProcessor : public BufferProcessorBase {
 public:
  SingleBufferProcessor() = default;

  [[nodiscard]] bool atBoundary() const override;
  [[nodiscard]] bool shouldStop() const override;

  void setStartFrame(size_t startFrame) {
    startFrame_ = startFrame;
  }

  void setEndFrame(size_t endFrame) {
    endFrame_ = endFrame;
  }

  void setLoop(bool loop) {
    loop_ = loop;
  }

  void setBuffer(const AudioBuffer *buffer) {
    buffer_ = buffer;
  }

 protected:
  CursorState advance(double rate) override;
  void consume(size_t frames) override;
  [[nodiscard]] size_t remainingInContiguousBlock() const override;
  [[nodiscard]] size_t currentIndex() const override;
  [[nodiscard]] const AudioBuffer *getBuffer() const override;
  [[nodiscard]] const AudioBuffer *getNextBuffer() const override;
  void handleBoundary() override;

 private:
  const AudioBuffer *buffer_ = nullptr;
  bool loop_ = false;
  size_t startFrame_ = 0;
  size_t endFrame_ = 0;
};

} // namespace audioapi
