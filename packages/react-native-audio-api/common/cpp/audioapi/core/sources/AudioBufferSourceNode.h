#pragma once

#include <audioapi/core/sources/AudioBufferBaseSourceNode.h>
#include <audioapi/libs/signalsmith-stretch/signalsmith-stretch.h>
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

  /// @note Audio Thread only
  void setBuffer(
      const std::shared_ptr<AudioBuffer> &buffer,
      const std::shared_ptr<DSPAudioBuffer> &audioBuffer);

  using AudioScheduledSourceNode::start;
  /// @note Audio Thread only
  void start(double when, double offset, double duration = -1);

  /// @note Audio Thread only
  void disable() override;

  /// @note Audio Thread only
  void setOnLoopEndedCallbackId(uint64_t callbackId);

  void unregisterOnLoopEndedCallback(uint64_t callbackId);

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

  uint64_t onLoopEndedCallbackId_ = 0; // 0 means no callback
  void sendOnLoopEndedEvent();

  double getVirtualStartFrame(float sampleRate) const;
  double getVirtualEndFrame(float sampleRate);

  std::unique_ptr<SingleBufferProcessor> processor_;
};

} // namespace audioapi
