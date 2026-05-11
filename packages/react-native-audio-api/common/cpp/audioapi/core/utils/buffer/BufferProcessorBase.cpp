#include <audioapi/core/utils/buffer/BufferProcessingDirection.h>
#include <audioapi/core/utils/buffer/BufferProcessorBase.h>
#include <audioapi/dsp/AudioUtils.hpp>
#include <audioapi/utils/AudioBuffer.hpp>
#include <audioapi/utils/Macros.h>
#include <algorithm>
#include <cstddef>
#include <memory>

namespace audioapi {

void BufferProcessorBase::process(
    const std::shared_ptr<DSPAudioBuffer> &output,
    size_t writeIndex,
    size_t framesLeft,
    double rate,
    bool interpolate) {
  setProcessingDirection(rate);
  if (interpolate) {
    renderInterpolated(output, writeIndex, framesLeft, rate);
  } else {
    renderBlock(output, writeIndex, framesLeft, rate);
  }
}

void BufferProcessorBase::setProcessingDirection(double rate) {
  direction_ = rate > 0.0 ? BufferProcessingDirection::FORWARD : BufferProcessingDirection::REVERSE;
}

bool BufferProcessorBase::shouldProcessFurther() {
  if (!atBoundary()) {
    return true; // Keep going
  }

  handleBoundary();

  // Check if we should loop
  return !shouldStop();
}

void BufferProcessorBase::renderBlock(
    const std::shared_ptr<DSPAudioBuffer> &output,
    size_t writeIndex,
    size_t framesLeft,
    double rate) {
  while (framesLeft > 0) {
    const size_t toCopy = std::min(remainingInContiguousBlock(), framesLeft);

    if (toCopy > 0) {
      const AudioBuffer *buffer = getBuffer().get();
      const size_t readIndex = currentIndex();

      if (direction_ == BufferProcessingDirection::REVERSE) {
        for (size_t ch = 0; ch < output->getNumberOfChannels(); ++ch) {
          output->getChannel(ch)->copyReverse(
              *buffer->getChannel(ch), readIndex, writeIndex, toCopy);
        }
      } else {
        output->copy(*buffer, readIndex, writeIndex, toCopy);
      }

      consume(toCopy);
      writeIndex += toCopy;
      framesLeft -= toCopy;
    }

    if (!shouldProcessFurther()) {
      output->zero(writeIndex, framesLeft);
      break;
    }
  }
}

void BufferProcessorBase::renderInterpolated(
    const std::shared_ptr<DSPAudioBuffer> &output,
    size_t writeIndex,
    size_t framesLeft,
    double rate) {
  const size_t numChannels = output->getNumberOfChannels();
  for (size_t i = 0; i < framesLeft; ++i) {
    const CursorState state = advance(rate);
    if (state.atEndOfBuffer) {
      output->zero(writeIndex, framesLeft - i);
      return;
    }

    const AudioBuffer *currentBuffer = getBuffer().get();
    for (size_t ch = 0; ch < numChannels; ++ch) {
      auto destination = output->getChannel(ch)->span();
      auto source = currentBuffer->getChannel(ch)->span();
      if (state.isCrossBuffer) {
        auto nextSource = getNextBuffer()->getChannel(ch)->span();
        const float currentSample = source[state.index];
        const float nextSample = nextSource[state.nextIndex];
        destination[writeIndex] = currentSample + state.factor * (nextSample - currentSample);
      } else {
        destination[writeIndex] =
            dsp::linearInterpolate(source, state.index, state.nextIndex, state.factor);
      }
    }

    writeIndex++;

    if (!shouldProcessFurther()) {
      output->zero(writeIndex, framesLeft - i - 1);
      return;
    }
  }
}
}; // namespace audioapi
