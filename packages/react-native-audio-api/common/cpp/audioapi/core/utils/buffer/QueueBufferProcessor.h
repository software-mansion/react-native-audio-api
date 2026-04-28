#pragma once

#include <audioapi/core/utils/buffer/BufferProcessorBase.hpp>
#include <audioapi/utils/AudioBuffer.hpp>

#include <cstddef>
#include <functional>
#include <list>
#include <memory>
#include <utility>

namespace audioapi {

class QueueBufferProcessor : public BufferProcessorBase {
 public:
  using OnBufferConsumed = std::function<void(
      size_t bufferId,
      std::shared_ptr<AudioBuffer> buffer,
      bool isLastInQueue,
      bool fireBufferEndedEvent)>;

  QueueBufferProcessor(
      std::list<std::pair<size_t, std::shared_ptr<AudioBuffer>>> *buffers,
      double *vReadIndex,
      float rate,
      OnBufferConsumed onBufferConsumed = {});

  /// Arm an in-place tail buffer. When the main queue would drain during
  /// processing, handleBoundary() appends this tail instead of stopping.
  void setPendingTail(std::shared_ptr<AudioBuffer> tailBuffer) {
    pendingTailBuffer_ = std::move(tailBuffer);
  }

  [[nodiscard]] bool didConsumeTail() const {
    return tailConsumed_;
  }

  CursorState advance() override;
  void consume(size_t frames) override;
  size_t remainingInContiguousBlock() override;
  size_t currentIndex() override;
  const AudioBuffer *getBuffer() override;
  const AudioBuffer *getNextBuffer() override;
  bool atBoundary() override;
  bool shouldStop() override;
  void handleBoundary() override;

 private:
  std::list<std::pair<size_t, std::shared_ptr<AudioBuffer>>> *buffers_;
  OnBufferConsumed onBufferConsumed_;
  std::shared_ptr<AudioBuffer> pendingTailBuffer_ = nullptr;
  bool tailConsumed_ = false;
};

} // namespace audioapi
