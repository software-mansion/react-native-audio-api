#pragma once

#include <audioapi/core/utils/buffer/BufferProcessingDirection.h>
#include <audioapi/core/utils/buffer/BufferProcessorBase.h>

#include <audioapi/utils/AudioBuffer.hpp>
#include <cstddef>
#include <memory>
#include <utility>

namespace audioapi {

/// @brief Buffer processor that handles a single audio buffer.
/// @note Before processing, the buffer, start frame, and end frame must be set. Looping can also be enabled if desired.
class SingleBufferProcessor : public BufferProcessorBase {
 public:
  SingleBufferProcessor() = default;

  [[nodiscard]] bool atBoundary() const override;
  [[nodiscard]] bool shouldStop() const override;

  /// @brief Set the starting frame for processing.
  void setStartFrame(size_t startFrame) {
    startFrame_ = startFrame;
  }

  /// @brief Set the ending frame for processing.
  void setEndFrame(size_t endFrame) {
    endFrame_ = endFrame;
  }

  /// @brief Enable or disable looping for this buffer processor.
  void setLoop(bool loop) {
    loop_ = loop;
  }

  /// @brief Set the audio buffer to process.
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
