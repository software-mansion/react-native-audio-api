#pragma once

#include <audioapi/core/sources/AudioBufferBaseSourceNode.h>
#include <audioapi/utils/AudioBuffer.hpp>

#include <audioapi/core/utils/buffer/SingleBufferProcessor.h>
#include <cstddef>
#include <memory>

namespace audioapi {

class AudioParam;
struct AudioBufferSourceOptions;

class AudioBufferSourceNode : public AudioBufferBaseSourceNode {
 public:
  explicit AudioBufferSourceNode(
      const std::shared_ptr<BaseAudioContext> &context,
      const AudioBufferSourceOptions &options);

  /// @note Audio Thread only
  void setLoop(bool loop);

  /// @note Audio Thread only
  void setLoopSkip(bool loopSkip);

  /// @note Audio Thread only
  void setLoopStart(double loopStart);

  /// @note Audio Thread only
  void setLoopEnd(double loopEnd);
  [[nodiscard]] double getLoopEnd() const {
    return loopEnd_;
  }

  /// @brief Hands the current buffers to the disposer and installs the new ones. Serves
  /// both a JS `buffer` assignment and the "acquire the content" refresh on start().
  /// Loop bounds are left untouched: the effective loop end is resolved against the
  /// buffer at render time, so a stale `loopEnd_` cannot outrun a shorter buffer.
  /// @note Audio Thread only
  void setBuffer(
      const std::shared_ptr<AudioBuffer> &buffer,
      const std::shared_ptr<DSPAudioBuffer> &audioBuffer);

  using AudioScheduledSourceNode::start;
  /// @note Audio Thread only
  void start(double when, double offset, double duration = -1);

  /// @note Audio Thread only
  void disable() override;

  void assignOnLoopEndedCallbackId(uint64_t callbackId);

 protected:
  double getCurrentPosition() const final;

  bool isEmpty() const final;

  void runBufferProcessor(
      const std::shared_ptr<DSPAudioBuffer> &processingBuffer,
      size_t startOffset,
      size_t offsetLength,
      float playbackRate,
      bool interpolate) final;

 private:
  // Looping related properties
  bool loop_;
  bool loopSkip_;
  double loopStart_;
  double loopEnd_;

  // User provided buffer
  std::shared_ptr<AudioBuffer> buffer_;

  EventCaller<AudioEvent::LOOP_ENDED> onLoopEndedEvent_;
  void sendOnLoopEndedEvent();

  double getVirtualStartFrame(float sampleRate) const;
  double getVirtualEndFrame(float sampleRate);

  std::unique_ptr<SingleBufferProcessor> processor_;
};

} // namespace audioapi
