#pragma once

#include <memory>

#include "AudioBuffer.h"
#include "AudioScheduledSourceNode.h"

namespace audioapi {

class AudioBus;
class AudioParam;

class AudioBufferSourceNode : public AudioScheduledSourceNode {
 public:
  explicit AudioBufferSourceNode(BaseAudioContext *context);

  [[nodiscard]] bool getLoop() const;
  [[nodiscard]] double getLoopStart() const;
  [[nodiscard]] double getLoopEnd() const;

  [[nodiscard]] std::shared_ptr<AudioParam> getDetuneParam() const;
  [[nodiscard]] std::shared_ptr<AudioParam> getPlaybackRateParam() const;

  [[nodiscard]] std::shared_ptr<AudioBuffer> getBuffer() const;

  void setLoop(bool loop);
  void setLoopStart(double loopStart);
  void setLoopEnd(double loopEnd);

  void setBuffer(const std::shared_ptr<AudioBuffer> &buffer);

 protected:
  void processNode(AudioBus *processingBus, int framesToProcess) override;

 private:
  // Looping related properties
  bool loop_;
  double loopStart_;
  double loopEnd_;

  // playback rate aka pitch change params
  std::shared_ptr<AudioParam> detuneParam_;
  std::shared_ptr<AudioParam> playbackRateParam_;

  // internal helper
  double vReadIndex_;

  // User provided buffer
  std::shared_ptr<AudioBuffer> buffer_;
  std::shared_ptr<AudioBus> alignedBus_;

  float getPlaybackRateValue(size_t &startOffset);

  double getVirtualStartFrame();
  double getVirtualEndFrame();

  void processWithoutInterpolation(
      AudioBus *processingBus,
      size_t startOffset,
      size_t offsetLength,
      float playbackRate);

  void processWithInterpolation(
      AudioBus *processingBus,
      size_t startOffset,
      size_t offsetLength,
      float playbackRate);
};

} // namespace audioapi
