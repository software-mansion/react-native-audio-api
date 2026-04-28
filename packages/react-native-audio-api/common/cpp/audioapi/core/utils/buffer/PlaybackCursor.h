#pragma once

#include <audioapi/utils/AudioBuffer.hpp>
#include <cstddef>

namespace audioapi {

struct CursorState {
  size_t index;
  size_t nextIndex;
  float factor;
  bool atEndOfBuffer;
  bool isCrossBuffer;
};

template <typename T>
concept PlaybackCursor = requires(T c, size_t n) {
  { c.advance() } -> std::same_as<CursorState>;
  { c.consume(n) } -> std::same_as<void>;
  { c.remainingInContiguousBlock() } -> std::same_as<size_t>;
  { c.currentIndex() } -> std::same_as<size_t>;
  { c.getBuffer() } -> std::same_as<const AudioBuffer *>;
  { c.getNextBuffer() } -> std::same_as<const AudioBuffer *>;
  { c.atBoundary() } -> std::same_as<bool>;
  { c.shouldStop() } -> std::same_as<bool>;
  { c.handleBoundary() } -> std::same_as<void>;
};

}; // namespace audioapi
