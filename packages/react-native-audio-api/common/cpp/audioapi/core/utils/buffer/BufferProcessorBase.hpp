#pragma once

#include <audioapi/dsp/AudioUtils.hpp>
#include <audioapi/utils/AudioBuffer.hpp>
#include <audioapi/utils/Macros.h>
#include <algorithm>
#include <cstddef>
#include <memory>

namespace audioapi {

struct CursorState {
  size_t index = 0;
  size_t nextIndex = 0;
  float factor = 0.0f;
  bool atEndOfBuffer = false;
  bool isCrossBuffer = false;
};

class BufferProcessorBase {
 public:
  virtual ~BufferProcessorBase() = default;
  BufferProcessorBase() = delete;
  DELETE_COPY_AND_MOVE(BufferProcessorBase);

  BufferProcessorBase(double position, float rate) : position_(position), rate_(rate) {}

  void process(
      const std::shared_ptr<DSPAudioBuffer> &output,
      size_t writeIndex,
      size_t framesLeft,
      bool interpolate) {
    if (interpolate) {
      renderInterpolated(output, writeIndex, framesLeft);
    } else {
      renderBlock(output, writeIndex, framesLeft);
    }
  }

  [[nodiscard]] double getPosition() const {
    return position_;
  }

  [[nodiscard]] virtual bool atBoundary() const = 0;
  [[nodiscard]] virtual bool shouldStop() const = 0;

 protected:
  virtual CursorState advance() = 0;
  virtual void consume(size_t frames) = 0;
  [[nodiscard]] virtual size_t remainingInContiguousBlock() const = 0;
  [[nodiscard]] virtual size_t currentIndex() const = 0;
  [[nodiscard]] virtual const AudioBuffer *getBuffer() const = 0;
  [[nodiscard]] virtual const AudioBuffer *getNextBuffer() const = 0;
  virtual void handleBoundary() = 0;
  [[nodiscard]] virtual bool isReverse() const {
    return false;
  }

  double position_;
  float rate_;

 private:
  void
  renderBlock(const std::shared_ptr<DSPAudioBuffer> &output, size_t writeIndex, size_t framesLeft) {
    while (framesLeft > 0) {
      const size_t toCopy = std::min(remainingInContiguousBlock(), framesLeft);

      if (toCopy > 0) {
        const AudioBuffer *buffer = getBuffer();
        const size_t readIndex = currentIndex();

        if (isReverse()) {
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

      if (atBoundary()) {
        handleBoundary();
        if (shouldStop()) {
          output->zero(writeIndex, framesLeft);
          break;
        }
      }
    }
  }

  void renderInterpolated(
      const std::shared_ptr<DSPAudioBuffer> &output,
      size_t writeIndex,
      size_t framesLeft) {
    const size_t numChannels = output->getNumberOfChannels();
    for (size_t i = 0; i < framesLeft; ++i) {
      const CursorState state = advance();
      if (state.atEndOfBuffer) {
        output->zero(writeIndex, framesLeft - i);
        return;
      }

      const AudioBuffer *currentBuffer = getBuffer();
      // If processing multiple buffers
      const AudioBuffer *nextBuffer = state.isCrossBuffer ? getNextBuffer() : currentBuffer;
      for (size_t ch = 0; ch < numChannels; ++ch) {
        auto destination = output->getChannel(ch)->span();
        auto source = currentBuffer->getChannel(ch)->span();
        if (state.isCrossBuffer) {
          auto nextSource = nextBuffer->getChannel(ch)->span();
          const float currentSample = source[state.index];
          const float nextSample = nextSource[state.nextIndex];
          destination[writeIndex] = currentSample + state.factor * (nextSample - currentSample);
        } else {
          destination[writeIndex] =
              dsp::linearInterpolate(source, state.index, state.nextIndex, state.factor);
        }
      }
      writeIndex++;

      if (atBoundary()) {
        handleBoundary();
        if (shouldStop()) {
          output->zero(writeIndex, framesLeft - i - 1);
          return;
        }
      }
    }
  }
};

} // namespace audioapi
