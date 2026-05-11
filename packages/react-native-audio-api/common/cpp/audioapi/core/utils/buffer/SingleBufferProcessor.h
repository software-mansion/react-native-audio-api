#pragma once

#include <audioapi/core/utils/buffer/BufferProcessingDirection.h>
#include <audioapi/core/utils/buffer/BufferProcessorBase.h>

#include <audioapi/utils/AudioBuffer.hpp>
#include <cstddef>
#include <memory>
#include <utility>

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

  void setBuffer(std::shared_ptr<const AudioBuffer> buffer) {
    buffer_ = std::move(buffer);
  }

 protected:
  CursorState advance(double rate) override;
  void consume(size_t frames) override;
  [[nodiscard]] size_t remainingInContiguousBlock() const override;
  [[nodiscard]] size_t currentIndex() const override;
  [[nodiscard]] std::shared_ptr<const AudioBuffer> getBuffer() const override;
  [[nodiscard]] std::shared_ptr<const AudioBuffer> getNextBuffer() const override;
  void handleBoundary() override;

 private:
  std::shared_ptr<const AudioBuffer> buffer_ = nullptr;
  bool loop_ = false;
  size_t startFrame_ = 0;
  size_t endFrame_ = 0;
};

} // namespace audioapi
