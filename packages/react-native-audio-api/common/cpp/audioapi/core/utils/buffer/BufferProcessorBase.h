#pragma once

#include <audioapi/core/utils/buffer/BufferProcessingDirection.h>
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
  DELETE_COPY_AND_MOVE(BufferProcessorBase);

  void process(
      const std::shared_ptr<DSPAudioBuffer> &output,
      size_t writeIndex,
      size_t framesLeft,
      double rate,
      bool interpolate);

  [[nodiscard]] double getPosition() const {
    return position_;
  }

  void setPosition(double position) {
    position_ = position;
  }

  [[nodiscard]] virtual bool atBoundary() const = 0;
  [[nodiscard]] virtual bool shouldStop() const = 0;

 protected:
  BufferProcessorBase() = default;

  virtual CursorState advance(double rate) = 0;
  virtual void consume(size_t frames) = 0;
  [[nodiscard]] virtual size_t remainingInContiguousBlock() const = 0;
  [[nodiscard]] virtual size_t currentIndex() const = 0;
  [[nodiscard]] virtual std::shared_ptr<const AudioBuffer> getBuffer() const = 0;
  [[nodiscard]] virtual std::shared_ptr<const AudioBuffer> getNextBuffer() const = 0;
  virtual void handleBoundary() = 0;

  double position_ = 0;
  BufferProcessingDirection direction_ = BufferProcessingDirection::FORWARD;

 private:
  void setProcessingDirection(double rate);

  void renderBlock(
      const std::shared_ptr<DSPAudioBuffer> &output,
      size_t writeIndex,
      size_t framesLeft,
      double rate);

  void renderInterpolated(
      const std::shared_ptr<DSPAudioBuffer> &output,
      size_t writeIndex,
      size_t framesLeft,
      double rate);

  [[nodiscard]] bool shouldProcessFurther();
};

} // namespace audioapi
