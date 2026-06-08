#include <audioapi/core/utils/buffer/QueueBufferProcessor.h>

#include <cstddef>
#include <iterator>
#include <list>
#include <memory>
#include <utility>

namespace audioapi {

QueueBufferProcessor::QueueBufferProcessor(
    std::list<std::pair<size_t, std::shared_ptr<AudioBuffer>>> *buffers,
    OnBufferConsumed onBufferConsumed)
    : buffers_(buffers), onBufferConsumed_(std::move(onBufferConsumed)) {}

CursorState QueueBufferProcessor::advance(double rate) {
  if (buffers_->empty()) {
    return {.atEndOfBuffer = true};
  }

  const size_t bufferSize = buffers_->front().second->getSize();
  const double currentPos = position_;
  const auto index = static_cast<size_t>(currentPos);
  const auto factor = static_cast<float>(currentPos - static_cast<double>(index));

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
      atEnd = (currentPos + rate) >= static_cast<double>(bufferSize);
    }
  }

  position_ += rate;

  return {
      .index = index,
      .nextIndex = nextIndex,
      .factor = factor,
      .atEndOfBuffer = atEnd,
      .isCrossBuffer = isCrossBuffer};
}

void QueueBufferProcessor::consume(size_t frames) {
  position_ += static_cast<double>(frames);
}

size_t QueueBufferProcessor::remainingInContiguousBlock() const {
  if (buffers_->empty()) {
    return 0;
  }
  const size_t size = buffers_->front().second->getSize();
  const size_t pos = currentIndex();
  return pos < size ? size - pos : 0;
}

size_t QueueBufferProcessor::currentIndex() const {
  return static_cast<size_t>(position_);
}

std::shared_ptr<const AudioBuffer> QueueBufferProcessor::getBuffer() const {
  if (buffers_->empty()) {
    return nullptr;
  }
  return buffers_->front().second;
}

std::shared_ptr<const AudioBuffer> QueueBufferProcessor::getNextBuffer() const {
  if (buffers_->empty()) {
    return pendingTailBuffer_;
  }
  auto it = std::next(buffers_->begin());
  if (it != buffers_->end()) {
    return it->second;
  }
  if (pendingTailBuffer_ != nullptr) {
    return pendingTailBuffer_;
  }
  return buffers_->front().second;
}

bool QueueBufferProcessor::atBoundary() const {
  if (buffers_->empty()) {
    return true;
  }
  return position_ >= static_cast<double>(buffers_->front().second->getSize());
}

bool QueueBufferProcessor::shouldStop() const {
  return buffers_->empty();
}

void QueueBufferProcessor::handleBoundary() {
  if (buffers_->empty()) {
    return;
  }

  auto bufferId = buffers_->front().first;
  auto buffer = std::move(buffers_->front().second);
  const auto consumedSize = static_cast<double>(buffer->getSize());
  buffers_->pop_front();
  position_ -= consumedSize;

  bool queueEmptyAfterPop = buffers_->empty();
  const bool willAppendTail = queueEmptyAfterPop && pendingTailBuffer_ != nullptr;
  bool fireBufferEndedEvent = !willAppendTail;

  if (onBufferConsumed_) {
    onBufferConsumed_(bufferId, buffer, queueEmptyAfterPop, fireBufferEndedEvent);
  }

  if (willAppendTail) {
    // Tail reuses the last real bufferId so the final onBufferEnded carries it.
    buffers_->emplace_back(bufferId, std::move(pendingTailBuffer_));
    pendingTailBuffer_ = nullptr;
    tailConsumed_ = true;
  }
}

} // namespace audioapi
