#pragma once

#include <audioapi/dsp/AudioUtils.hpp>
#include <audioapi/utils/AudioBuffer.hpp>
#include <audioapi/utils/Macros.h>
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
      bool interpolate);

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
  renderBlock(const std::shared_ptr<DSPAudioBuffer> &output, size_t writeIndex, size_t framesLeft);

  void renderInterpolated(
      const std::shared_ptr<DSPAudioBuffer> &output,
      size_t writeIndex,
      size_t framesLeft);
};

} // namespace audioapi
