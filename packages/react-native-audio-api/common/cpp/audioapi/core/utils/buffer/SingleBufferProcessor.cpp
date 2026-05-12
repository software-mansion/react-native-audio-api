#include <audioapi/core/utils/buffer/SingleBufferProcessor.h>

#include <cmath>
#include <cstddef>
#include <memory>

namespace audioapi {

CursorState SingleBufferProcessor::advance(double rate) {
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

  return {.index = index, .nextIndex = nextIndex, .factor = factor, .atEndOfBuffer = atEnd};
}

size_t SingleBufferProcessor::remainingInContiguousBlock() const {
  if (atBoundary()) {
    return 0;
  }
  if (direction_ == BufferProcessingDirection::REVERSE) {
    // +1 because we read down to and including startFrame_
    return currentIndex() - startFrame_ + 1;
  }
  return endFrame_ - currentIndex();
}

void SingleBufferProcessor::consume(size_t frames) {
  if (direction_ == BufferProcessingDirection::REVERSE) {
    position_ -= static_cast<double>(frames);
  } else {
    position_ += static_cast<double>(frames);
  }
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
  return position_ < static_cast<double>(startFrame_) ||
      position_ >= static_cast<double>(endFrame_);
}

bool SingleBufferProcessor::shouldStop() const {
  return !loop_;
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
  } else if (position_ < static_cast<double>(startFrame_)) {
    position_ += range;
  }
}

} // namespace audioapi
