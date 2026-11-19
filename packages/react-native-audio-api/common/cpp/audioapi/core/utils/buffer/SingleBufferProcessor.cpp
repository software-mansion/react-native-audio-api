#include <audioapi/core/utils/buffer/SingleBufferProcessor.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <memory>

namespace audioapi {

CursorState SingleBufferProcessor::advance(double rate) {
  if (isFrameBudgetSpent()) {
    return {.atEndOfBuffer = true};
  }

  const double currentPosition = position_;
  const auto index = static_cast<size_t>(currentPosition);
  const auto factor = static_cast<float>(currentPosition - static_cast<double>(index));

  size_t nextIndex;
  if (direction_ == BufferProcessingDirection::FORWARD) {
    nextIndex = index + 1;
    if (nextIndex >= endFrame_) {
      nextIndex = loop_ ? startFrame_ : index;
    }
  } else { // REVERSE — interpolate toward the previous sample.
    if (index > startFrame_) {
      nextIndex = index - 1;
    } else {
      nextIndex = loop_ ? endFrame_ - 1 : index;
    }
  }

  position_ += rate;

  const bool atEnd = shouldStop() &&
      (currentPosition >= static_cast<double>(endFrame_) ||
       currentPosition < static_cast<double>(startFrame_));

  spendFrameBudget(std::fabs(rate));

  return {.index = index, .nextIndex = nextIndex, .factor = factor, .atEndOfBuffer = atEnd};
}

size_t SingleBufferProcessor::remainingInContiguousBlock() const {
  if (atBoundary()) {
    return 0;
  }
  // +1 in reverse because we read down to and including startFrame_
  const size_t framesToEdge = direction_ == BufferProcessingDirection::REVERSE
      ? currentIndex() - startFrame_ + 1
      : endFrame_ - currentIndex();
  if (!frameBudget_) {
    return framesToEdge;
  }
  return std::min(framesToEdge, static_cast<size_t>(std::ceil(*frameBudget_)));
}

void SingleBufferProcessor::consume(size_t frames) {
  if (direction_ == BufferProcessingDirection::REVERSE) {
    position_ -= static_cast<double>(frames);
  } else {
    position_ += static_cast<double>(frames);
  }
  spendFrameBudget(static_cast<double>(frames));
}

size_t SingleBufferProcessor::currentIndex() const {
  return static_cast<size_t>(std::floor(position_));
}

std::shared_ptr<const AudioBuffer> SingleBufferProcessor::getBuffer() const {
  return buffer_;
}

std::shared_ptr<const AudioBuffer> SingleBufferProcessor::getNextBuffer() const {
  return buffer_;
}

bool SingleBufferProcessor::atBoundary() const {
  return isFrameBudgetSpent() || position_ < static_cast<double>(startFrame_) ||
      position_ >= static_cast<double>(endFrame_);
}

bool SingleBufferProcessor::shouldStop() const {
  return !loop_ || isFrameBudgetSpent();
}

void SingleBufferProcessor::handleBoundary() {
  if (shouldStop()) {
    return;
  }

  if (endFrame_ <= startFrame_) {
    return;
  }
  const auto range = static_cast<double>(endFrame_ - startFrame_);

  if (position_ >= static_cast<double>(endFrame_)) {
    position_ -= range;
    if (direction_ == BufferProcessingDirection::FORWARD && onLoopEnded_) {
      onLoopEnded_();
    }
  } else if (position_ < static_cast<double>(startFrame_)) {
    position_ += range;
    if (direction_ == BufferProcessingDirection::REVERSE && onLoopEnded_) {
      onLoopEnded_();
    }
  }
}

} // namespace audioapi
