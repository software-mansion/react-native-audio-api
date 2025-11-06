#pragma once

#include <audioapi/core/sources/AudioBuffer.h>
#include <audioapi/core/sources/AudioBufferBaseSourceNode.h>
#include <audioapi/libs/signalsmith-stretch/signalsmith-stretch.h>

#include <memory>
#include <cstddef>
#include <algorithm>
#include <string>

namespace audioapi {

class AudioBus;
class AudioParam;

class AudioBufferSourceNode : public AudioBufferBaseSourceNode {
 public:
  explicit AudioBufferSourceNode(BaseAudioContext *context, bool pitchCorrection);
  ~AudioBufferSourceNode() override;

  [[nodiscard]] bool getLoop() const;
  [[nodiscard]] bool getLoopSkip() const;
  [[nodiscard]] double getLoopStart() const;
  [[nodiscard]] double getLoopEnd() const;
  [[nodiscard]] std::shared_ptr<AudioBuffer> getBuffer() const;

  void setLoop(bool loop);
  void setLoopSkip(bool loopSkip);
  void setLoopStart(double loopStart);
  void setLoopEnd(double loopEnd);
  void setBuffer(const std::shared_ptr<AudioBuffer> &buffer);

  void start(double when, double offset, double duration = -1);
  void disable() override;

  void setOnLoopEndedCallbackId(uint64_t callbackId);

 protected:
  std::shared_ptr<AudioBus> processNode(const std::shared_ptr<AudioBus>& processingBus, int framesToProcess) override;
  double getCurrentPosition() const override;

 private:
  // Looping related properties
  bool loop_;
  bool loopSkip_;
  double loopStart_;
  double loopEnd_;

  // User provided buffer
  std::shared_ptr<AudioBuffer> buffer_;
  std::shared_ptr<AudioBus> alignedBus_;

  std::atomic<uint64_t> onLoopEndedCallbackId_ = 0; // 0 means no callback
  void sendOnLoopEndedEvent();

  void processWithoutInterpolation(
      const std::shared_ptr<AudioBus>& processingBus,
      size_t startOffset,
      size_t offsetLength,
      float playbackRate) override;

  void processWithInterpolation(
      const std::shared_ptr<AudioBus>& processingBus,
      size_t startOffset,
      size_t offsetLength,
      float playbackRate) override;

  double getVirtualStartFrame();
  double getVirtualEndFrame();
};

} // namespace audioapi
