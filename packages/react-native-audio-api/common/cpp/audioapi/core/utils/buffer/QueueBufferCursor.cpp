#include <audioapi/core/utils/buffer/QueueBufferCursor.h>

#include <cmath>
#include <cstddef>
#include <iterator>
#include <utility>

namespace audioapi {

CursorState QueueBufferCursor::advance() {
  if (buffers_->empty()) {
    return {
        .index = 0, .nextIndex = 0, .factor = 0.0f, .atEndOfBuffer = true, .isCrossBuffer = false};
  }

  const auto *currentBuffer = buffers_->front().second.get();
  const size_t bufferSize = currentBuffer->getSize();

  double currentPos = *vReadIndex_;
  auto index = static_cast<size_t>(currentPos);
  auto factor = static_cast<float>(currentPos - static_cast<double>(index));

  size_t nextIndex = index + 1;
  bool isCrossBuffer = false;
  bool atEnd = false;

  if (nextIndex >= bufferSize) {
    const bool hasFollowUp =
        std::next(buffers_->begin()) != buffers_->end() || pendingTailBuffer_ != nullptr;

    if (hasFollowUp) {
      nextIndex = 0;
      isCrossBuffer = true;
    } else {
      nextIndex = index;
      atEnd = (currentPos + std::fabs(rate_)) >= static_cast<double>(bufferSize);
    }
  }

  *vReadIndex_ += std::fabs(rate_);

  return {
      .index = index,
      .nextIndex = nextIndex,
      .factor = factor,
      .atEndOfBuffer = atEnd,
      .isCrossBuffer = isCrossBuffer};
}

size_t QueueBufferCursor::remainingInContiguousBlock() const {
  if (buffers_->empty()) {
    return 0;
  }
  const size_t size = buffers_->front().second->getSize();
  const size_t pos = currentIndex();
  return pos < size ? size - pos : 0;
}

void QueueBufferCursor::consume(size_t frames) {
  *vReadIndex_ += static_cast<double>(frames);
}

size_t QueueBufferCursor::currentIndex() const {
  return static_cast<size_t>(*vReadIndex_);
}

const AudioBuffer *QueueBufferCursor::getBuffer() const {
  if (buffers_->empty()) {
    return nullptr;
  }
  return buffers_->front().second.get();
}

const AudioBuffer *QueueBufferCursor::getNextBuffer() const {
  if (buffers_->empty()) {
    return pendingTailBuffer_ ? pendingTailBuffer_.get() : nullptr;
  }
  auto it = std::next(buffers_->begin());
  if (it != buffers_->end()) {
    return it->second.get();
  }
  if (pendingTailBuffer_ != nullptr) {
    return pendingTailBuffer_.get();
  }
  return buffers_->front().second.get();
}

bool QueueBufferCursor::atBoundary() const {
  if (buffers_->empty()) {
    return true;
  }
  return *vReadIndex_ >= static_cast<double>(buffers_->front().second->getSize());
}

bool QueueBufferCursor::shouldStop() const {
  return buffers_->empty();
}

void QueueBufferCursor::handleBoundary() {
  if (buffers_->empty()) {
    return;
  }

  auto bufferId = buffers_->front().first;
  auto buffer = std::move(buffers_->front().second);
  const auto consumedSize = static_cast<double>(buffer->getSize());
  buffers_->pop_front();
  *vReadIndex_ -= consumedSize;

  const bool queueEmptyAfterPop = buffers_->empty();
  const bool willAppendTail = queueEmptyAfterPop && pendingTailBuffer_ != nullptr;
  const bool fireBufferEndedEvent = !willAppendTail;

  if (onBufferConsumed_) {
    onBufferConsumed_(bufferId, std::move(buffer), queueEmptyAfterPop, fireBufferEndedEvent);
  }

  if (willAppendTail) {
    // Tail reuses the last real bufferId so the final onBufferEnded carries it.
    buffers_->emplace_back(bufferId, std::move(pendingTailBuffer_));
    pendingTailBuffer_ = nullptr;
    tailConsumed_ = true;
  }
}

double QueueBufferCursor::getVirtualReadIndex() const {
  return *vReadIndex_;
}

}; // namespace audioapi
