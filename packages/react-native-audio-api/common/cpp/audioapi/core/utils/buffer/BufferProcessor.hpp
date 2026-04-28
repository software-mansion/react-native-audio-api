#pragma once

#include <audioapi/core/utils/buffer/PlaybackCursor.h>
#include <audioapi/dsp/AudioUtils.hpp>
#include <algorithm>
#include <memory>
namespace audioapi {

template <PlaybackCursor T>
class BufferProcessor {
 public:
  static void process(
      T &cursor,
      const std::shared_ptr<DSPAudioBuffer> &output,
      size_t startOffset,
      size_t length,
      bool interpolate) {
    if (interpolate) {
      renderInterpolated(cursor, output, startOffset, length);
    } else {
      renderBlock(cursor, output, startOffset, length);
    }
  }

 private:
  static void renderBlock(
      T &cursor,
      const std::shared_ptr<DSPAudioBuffer> &output,
      size_t writeIndex,
      size_t framesLeft) {
    while (framesLeft > 0) {
      size_t available = cursor.remainingInContiguousBlock();
      size_t toCopy = std::min(available, framesLeft);

      if (toCopy > 0) {
        output->copy(*cursor.getBuffer(), cursor.currentIndex(), writeIndex, toCopy);
        cursor.consume(toCopy);
        writeIndex += toCopy;
        framesLeft -= toCopy;
      }

      if (cursor.atBoundary()) {
        cursor.handleBoundary(); // Wraps index for loop, or pops for queue.
        if (cursor.shouldStop()) {
          output->zero(writeIndex, framesLeft);
          return;
        }
      }
    }
  }

  static void renderInterpolated(
      T &cursor,
      const std::shared_ptr<DSPAudioBuffer> &output,
      size_t writeIndex,
      size_t framesLeft) {
    for (size_t i = 0; i < framesLeft; ++i) {
      CursorState state = cursor.advance();
      if (state.atEndOfBuffer) {
        output->zero(writeIndex, framesLeft - i);
        return;
      }

      for (size_t ch = 0; ch < output->getNumberOfChannels(); ++ch) {
        auto destination = output->getChannel(ch)->span();
        auto source = cursor.getBuffer()->getChannel(ch)->span();

        if (state.isCrossBuffer) {
          auto nextSource = cursor.getNextBuffer()->getChannel(ch)->span();
          float currentSample = source[state.index];
          float nextSample = nextSource[state.nextIndex];
          destination[writeIndex] = currentSample + state.factor * (nextSample - currentSample);
        } else {
          destination[writeIndex] =
              dsp::linearInterpolate(source, state.index, state.nextIndex, state.factor);
        }
      }
      writeIndex++;

      if (cursor.atBoundary()) {
        cursor.handleBoundary(); // Wraps index for loop, or pops for queue.
        if (cursor.shouldStop()) {
          output->zero(writeIndex, framesLeft - i - 1);
          return;
        }
      }
    }
  }
};

} // namespace audioapi
