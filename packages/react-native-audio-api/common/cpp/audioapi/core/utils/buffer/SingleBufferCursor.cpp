#pragma once

#include <audioapi/core/utils/buffer/PlaybackCursor.h>
#include <audioapi/core/utils/buffer/SingleBufferCursor.h>
#include <cstddef>

namespace audioapi {

CursorState SingleBufferCursor::advance() {
  double currentPosition = *vReadIndex_;
  auto index = static_cast<size_t>(currentPosition);

  auto factor = static_cast<float>(currentPosition - static_cast<double>(index));

  *vReadIndex_ += rate_;

  size_t nextIndex;
  if (direction_ == BufferProcessingDirection::FORWARD) {
    nextIndex = index + 1;
    if (nextIndex >= endFrame_) {
      nextIndex = loop_ ? startFrame_ : index;
    }
  } else { // REVERSE
    nextIndex = index > startFrame_ ? index - 1 : index;
    if (nextIndex < startFrame_) {
      nextIndex = loop_ ? endFrame_ - 1 : index;
    }
  }

  bool atEnd = !loop_ && (*vReadIndex_ >= static_cast<double>(endFrame_));

  return {.index = index, .nextIndex = nextIndex, .factor = factor, .atEndOfBuffer = atEnd};
}

size_t SingleBufferCursor::remainingInContiguousBlock() const {
  if (direction_ == BufferProcessingDirection::FORWARD) {
    return static_cast<size_t>(static_cast<double>(endFrame_) - *vReadIndex_);
  } // REVERSE
  return static_cast<size_t>(*vReadIndex_ - static_cast<double>(startFrame_));
}

void SingleBufferCursor::consume(size_t frames) {
  *vReadIndex_ +=
      static_cast<double>(frames) * (direction_ == BufferProcessingDirection::REVERSE ? -1.0 : 1.0);
}

size_t SingleBufferCursor::currentIndex() const {
  return static_cast<size_t>(*vReadIndex_);
}

const AudioBuffer *SingleBufferCursor::getBuffer() const {
  return buffer_;
}

const AudioBuffer *SingleBufferCursor::getNextBuffer() const {
  return buffer_;
}

bool SingleBufferCursor::atBoundary() const {
  return *vReadIndex_ >= static_cast<double>(endFrame_) ||
      *vReadIndex_ < static_cast<double>(startFrame_);
}

bool SingleBufferCursor::shouldStop() const {
  return !loop_;
}

void SingleBufferCursor::handleBoundary() {
  auto range = static_cast<double>(endFrame_ - startFrame_);

  if (range <= 0) {
    return;
  }

  if (*vReadIndex_ >= static_cast<double>(endFrame_)) {
    *vReadIndex_ -= range;
  } else if (*vReadIndex_ < static_cast<double>(startFrame_)) {
    *vReadIndex_ += range;
  }
}

}; // namespace audioapi