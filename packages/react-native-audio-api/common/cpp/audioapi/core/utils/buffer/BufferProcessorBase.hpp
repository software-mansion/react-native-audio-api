#pragma once

#include <audioapi/core/utils/buffer/BufferProcessingDirection.hpp>
#include <audioapi/dsp/AudioUtils.hpp>
#include <audioapi/utils/AudioBuffer.hpp>
#include <algorithm>
#include <cstddef>
#include <memory>
namespace audioapi {

struct CursorState {
  size_t index;
  size_t nextIndex;
  float factor;
  bool atEndOfBuffer;
};

class BufferProcessorBase {

 public:
  virtual ~BufferProcessorBase() = default;
  BufferProcessorBase() = delete;
  BufferProcessorBase(const BufferProcessorBase &) = delete;
  BufferProcessorBase &operator=(const BufferProcessorBase &) = delete;
  BufferProcessorBase(BufferProcessorBase &&) = delete;
  BufferProcessorBase &operator=(BufferProcessorBase &&) = delete;
  BufferProcessorBase(
      AudioBuffer *buffer,
      double *vReadIndex,
      bool loop,
      float rate,
      size_t startFrame,
      size_t endFrame)
      : buffer_(buffer),
        vReadIndex_(vReadIndex),
        loop_(loop),
        rate_(rate),
        startFrame_(startFrame),
        endFrame_(endFrame),
        direction_(
            rate_ >= 0 ? BufferProcessingDirection::FORWARD : BufferProcessingDirection::REVERSE) {
        };

  virtual CursorState advance() = 0;
  virtual void consume(size_t n) = 0;
  virtual size_t remainingInContiguousBlock() = 0;
  virtual size_t currentIndex() = 0;
  virtual const AudioBuffer *getBuffer() = 0;
  virtual const AudioBuffer *getNextBuffer() = 0;
  virtual bool atBoundary() = 0;
  virtual bool shouldStop() = 0;
  virtual void handleBoundary() = 0;

  void process(
      const std::shared_ptr<DSPAudioBuffer> &output,
      size_t startOffset,
      size_t length,
      bool interpolate) {
    if (interpolate) {
      renderInterpolated(output, startOffset, length);
    } else {
      renderBlock(output, startOffset, length);
    }
  };
  void
  renderBlock(const std::shared_ptr<DSPAudioBuffer> &output, size_t writeIndex, size_t framesLeft) {
    while (framesLeft > 0) {
      size_t available = remainingInContiguousBlock();
      size_t toCopy = std::min(available, framesLeft);

      if (toCopy > 0) {
        output->copy(*getBuffer(), currentIndex(), writeIndex, toCopy);
        consume(toCopy);
        writeIndex += toCopy;
        framesLeft -= toCopy;
      }

      if (atBoundary()) {
        if (shouldStop()) {
          output->zero(writeIndex, framesLeft);
          break;
        }
        handleBoundary();
      }
    }
  }
  void renderInterpolated(
      const std::shared_ptr<DSPAudioBuffer> &output,
      size_t writeIndex,
      size_t framesLeft) {
    for (size_t i = 0; i < framesLeft; ++i) {
      CursorState state = advance();
      if (state.atEndOfBuffer) {
        output->zero(writeIndex, framesLeft - 1);
        break;
      }

      for (size_t ch = 0; ch < output->getNumberOfChannels(); ++ch) {
        auto destination = output->getChannel(ch)->span();
        auto source = getBuffer()->getChannel(ch)->span();
        destination[writeIndex] =
            dsp::linearInterpolate(source, state.index, state.nextIndex, state.factor);
      }
      writeIndex++;
    }
  }

 protected:
  const AudioBuffer *buffer_;
  double *vReadIndex_;
  bool loop_;
  float rate_;
  size_t startFrame_, endFrame_;
  BufferProcessingDirection direction_;
};

} // namespace audioapi