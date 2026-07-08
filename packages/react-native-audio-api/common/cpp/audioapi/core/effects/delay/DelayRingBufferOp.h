#pragma once

#include <audioapi/utils/AudioBuffer.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>

namespace audioapi::delay_ring {

enum class BufferAction : std::uint8_t { READ, WRITE };

/// Ring-buffer read/write: splits at the end of the ring, then advances the head with wrap.
inline void bufferOperation(
    const std::shared_ptr<AudioBuffer> &delayBuffer,
    const std::shared_ptr<DSPAudioBuffer> &processingBuffer,
    int framesToProcess,
    size_t &operationStartingIndex,
    BufferAction action) {
  size_t processingBufferStartIndex = 0;

  const size_t bufferSize = delayBuffer->getSize();
  if (operationStartingIndex + static_cast<size_t>(framesToProcess) > bufferSize) {
    const int tail = static_cast<int>(bufferSize - operationStartingIndex);

    if (action == BufferAction::WRITE) {
      delayBuffer->sum(*processingBuffer, processingBufferStartIndex, operationStartingIndex, tail);
    } else {
      processingBuffer->sum(*delayBuffer, operationStartingIndex, processingBufferStartIndex, tail);
      delayBuffer->zero(operationStartingIndex, static_cast<size_t>(tail));
    }

    operationStartingIndex = 0;
    processingBufferStartIndex += static_cast<size_t>(tail);
    framesToProcess -= tail;
  }

  if (action == BufferAction::WRITE) {
    delayBuffer->sum(
        *processingBuffer, processingBufferStartIndex, operationStartingIndex, framesToProcess);
    processingBuffer->zero();
  } else {
    processingBuffer->sum(
        *delayBuffer, operationStartingIndex, processingBufferStartIndex, framesToProcess);
    delayBuffer->zero(operationStartingIndex, static_cast<size_t>(framesToProcess));
  }

  operationStartingIndex =
      (operationStartingIndex + static_cast<size_t>(framesToProcess)) % bufferSize;
}

} // namespace audioapi::delay_ring
