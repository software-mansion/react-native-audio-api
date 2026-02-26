#pragma once

#include <audioapi/core/sources/AudioBufferBaseSourceNode.h>
#include <audioapi/libs/signalsmith-stretch/signalsmith-stretch.h>
#include <audioapi/utils/AudioBuffer.h>

#include <cstddef>
#include <memory>

namespace audioapi {

class AudioBuffer;
class AudioParam;
struct AudioBufferSourceOptions;

class AudioBufferSourceNode : public AudioBufferBaseSourceNode {
 public:
  explicit AudioBufferSourceNode(
      const std::shared_ptr<BaseAudioContext> &context,
      const AudioBufferSourceOptions &options);

  void setLoop(bool loop);
  void setLoopSkip(bool loopSkip);
  void setLoopStart(double loopStart);
  void setLoopEnd(double loopEnd);

  /// @note Buffer can be set (not to nullptr) only once.
  /// This is consistent with Web Audio API.
  void setBuffer(
      const std::shared_ptr<AudioBuffer> &buffer,
      const std::shared_ptr<AudioBuffer> &playbackRateBuffer,
      const std::shared_ptr<AudioBuffer> &audioBuffer);

  using AudioScheduledSourceNode::start;
  void start(double when, double offset, double duration = -1);
  void disable() override;

  void setOnLoopEndedCallbackId(uint64_t callbackId);

  /// @note Thread safe, because does not access state of the node.
  void unregisterOnLoopEndedCallback(uint64_t callbackId);

 protected:
  std::shared_ptr<AudioBuffer> processNode(
      const std::shared_ptr<AudioBuffer> &processingBuffer,
      int framesToProcess) override;
  double getCurrentPosition() const override;

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

  void processWithoutInterpolation(
      const std::shared_ptr<AudioBuffer> &processingBuffer,
      size_t startOffset,
      size_t offsetLength,
      float playbackRate) override;

  void processWithInterpolation(
      const std::shared_ptr<AudioBuffer> &processingBuffer,
      size_t startOffset,
      size_t offsetLength,
      float playbackRate) override;

  double getVirtualStartFrame(float sampleRate) const;
  double getVirtualEndFrame(float sampleRate);
};

} // namespace audioapi
