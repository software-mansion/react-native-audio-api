#include <audioapi/core/utils/buffer/SingleBufferProcessor.h>

#include <cstddef>

namespace audioapi {

CursorState SingleBufferProcessor::advance() {
  const double currentPosition = position_;
  const auto index = static_cast<size_t>(currentPosition);
  const auto factor = static_cast<float>(currentPosition - static_cast<double>(index));
  const auto startFrameIdx = static_cast<size_t>(startFrame_);
  const auto endFrameIdx = static_cast<size_t>(endFrame_);

  size_t nextIndex;
  if (direction_ == BufferProcessingDirection::FORWARD) {
    nextIndex = index + 1;
    if (nextIndex >= endFrameIdx) {
      nextIndex = loop_ ? startFrameIdx : index;
    }
  } else { // REVERSE — interpolate toward the previous sample.
    if (index > startFrameIdx) {
      nextIndex = index - 1;
    } else {
      nextIndex = loop_ ? endFrameIdx - 1 : index;
    }
  }

  position_ += rate_;

  const bool atEnd =
      shouldStop() && (currentPosition >= endFrame_ || currentPosition < startFrame_);

  return {.index = index, .nextIndex = nextIndex, .factor = factor, .atEndOfBuffer = atEnd};
}

size_t SingleBufferProcessor::remainingInContiguousBlock() const {
  if (direction_ == BufferProcessingDirection::REVERSE) {
    // +1 because we read down to and including the startFrame_
    return static_cast<size_t>(position_ - startFrame_) + 1;
  }
  return static_cast<size_t>(endFrame_ - position_);
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

const AudioBuffer *SingleBufferProcessor::getBuffer() const {
  return buffer_;
}

const AudioBuffer *SingleBufferProcessor::getNextBuffer() const {
  return buffer_;
}

bool SingleBufferProcessor::atBoundary() const {
  // For REVERSE we also treat position >= endFrame_ as a boundary so that callers
  // who set the cursor at the very end (e.g. start(when, offset=duration) with negative rate)
  // get wrapped/stopped before we ever try to read source[endFrame_] (out of bounds).
  return position_ < startFrame_ || position_ >= endFrame_;
}

bool SingleBufferProcessor::shouldStop() const {
  return !loop_;
}

void SingleBufferProcessor::handleBoundary() {
  if (shouldStop()) {
    return;
  }

  const auto range = endFrame_ - startFrame_;
  if (range <= 0) {
    return;
  }

  // Symmetric wrap: bring position back into [startFrame_, endFrame_) regardless of
  // direction. Covers FORWARD overflow, REVERSE underflow, and the REVERSE-overflow
  // edge case where the caller seeded the cursor at endFrame_.
  if (position_ >= endFrame_) {
    position_ -= range;
  } else if (position_ < startFrame_) {
    position_ += range;
  }
}

} // namespace audioapi
