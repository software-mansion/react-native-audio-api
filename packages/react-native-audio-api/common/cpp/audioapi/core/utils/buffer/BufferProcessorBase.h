#pragma once

#include <audioapi/core/utils/buffer/BufferProcessingDirection.h>
#include <audioapi/dsp/AudioUtils.hpp>
#include <audioapi/utils/AudioBuffer.hpp>
#include <audioapi/utils/Macros.h>
#include <cstddef>
#include <memory>

namespace audioapi {

/// @brief Struct to hold the state of the buffer cursor during processing.
struct CursorState {
  size_t index = 0;
  size_t nextIndex = 0;
  float factor = 0.0f;
  bool atEndOfBuffer = false;
  bool isCrossBuffer = false;
};

/// @brief Abstract base class for processing audio buffers, handling position tracking, looping, and interpolation logic.
class BufferProcessorBase {
 public:
  virtual ~BufferProcessorBase() = default;
  DELETE_COPY_AND_MOVE(BufferProcessorBase);

  /// @brief Process the audio buffer, applying looping and interpolation as needed.
  /// @param output The output buffer to write processed audio into.
  /// @param writeIndex The starting index in the output buffer to write to.
  /// @param framesLeft The number of frames left to process in this block.
  /// @param rate The playback rate, which may affect processing direction and interpolation.
  /// @param interpolate Whether to apply interpolation when processing.
  void process(
      const std::shared_ptr<DSPAudioBuffer> &output,
      size_t writeIndex,
      size_t framesLeft,
      double rate,
      bool interpolate);

  /// @brief Get the current position of the buffer cursor.
  /// @return The current position in frames.
  [[nodiscard]] double getPosition() const {
    return position_;
  }

  /// @brief Set the current position of the buffer cursor.
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
