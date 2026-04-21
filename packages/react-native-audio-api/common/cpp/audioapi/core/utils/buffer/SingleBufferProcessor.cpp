#pragma once

#include "audioapi/core/utils/buffer/SingleBufferProcessor.h"

namespace audioapi {

CursorState SingleBufferProcessor::advance() {
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
};
size_t SingleBufferProcessor::remainingInContiguousBlock() {
  if (direction_ == BufferProcessingDirection::FORWARD) {
    return static_cast<size_t>(static_cast<double>(endFrame_) - *vReadIndex_);
  } // REVERSE
  return static_cast<size_t>(*vReadIndex_ - static_cast<double>(startFrame_));
}

void SingleBufferProcessor::consume(size_t frames) {
  *vReadIndex_ +=
      static_cast<double>(frames) * (direction_ == BufferProcessingDirection::REVERSE ? -1.0 : 1.0);
}

size_t SingleBufferProcessor::currentIndex() {
  return static_cast<size_t>(*vReadIndex_);
}

const AudioBuffer *SingleBufferProcessor::getBuffer() {
  return buffer_;
}

const AudioBuffer *SingleBufferProcessor::getNextBuffer() {
  return buffer_;
}

bool SingleBufferProcessor::atBoundary() {
  return *vReadIndex_ >= static_cast<double>(endFrame_) ||
      *vReadIndex_ < static_cast<double>(startFrame_);
}

bool SingleBufferProcessor::shouldStop() {
  return !loop_;
}

void SingleBufferProcessor::handleBoundary() {
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

} // namespace audioapi