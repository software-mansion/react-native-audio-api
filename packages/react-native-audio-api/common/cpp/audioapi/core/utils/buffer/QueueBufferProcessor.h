#pragma once

#include <audioapi/core/utils/buffer/BufferProcessorBase.h>
#include <audioapi/utils/AudioBuffer.hpp>

#include <audioapi/utils/FatFunction.hpp>
#include <cstddef>
#include <list>
#include <memory>
#include <utility>

namespace audioapi {

inline constexpr size_t ON_BUFFER_CONSUMED_CALLBACK_SIZE = 64;

using OnBufferConsumed = FatFunction<
    ON_BUFFER_CONSUMED_CALLBACK_SIZE,
    void(size_t &, const std::shared_ptr<AudioBuffer> &, bool &, bool &)>;

class QueueBufferProcessor : public BufferProcessorBase {
 public:
  QueueBufferProcessor(
      std::list<std::pair<size_t, std::shared_ptr<AudioBuffer>>> *buffers,
      OnBufferConsumed onBufferConsumed = {});

  /// Arm an in-place tail buffer. When the main queue would drain during
  /// processing, handleBoundary() appends this tail instead of stopping.
  void setPendingTail(std::shared_ptr<AudioBuffer> tailBuffer) {
    pendingTailBuffer_ = std::move(tailBuffer);
  }

  [[nodiscard]] bool didConsumeTail() const {
    return tailConsumed_;
  }

  [[nodiscard]] bool atBoundary() const override;
  [[nodiscard]] bool shouldStop() const override;

 protected:
  CursorState advance(double rate) override;
  void consume(size_t frames) override;
  [[nodiscard]] size_t remainingInContiguousBlock() const override;
  [[nodiscard]] size_t currentIndex() const override;
  [[nodiscard]] std::shared_ptr<const AudioBuffer> getBuffer() const override;
  [[nodiscard]] std::shared_ptr<const AudioBuffer> getNextBuffer() const override;
  void handleBoundary() override;

 private:
  std::list<std::pair<size_t, std::shared_ptr<AudioBuffer>>> *buffers_;
  OnBufferConsumed onBufferConsumed_;
  std::shared_ptr<AudioBuffer> pendingTailBuffer_ = nullptr;
  bool tailConsumed_ = false;
};

} // namespace audioapi
