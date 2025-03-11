#pragma once

#include <audioapi/core/sources/AudioBuffer.h>
#include <audioapi/core/sources/AudioScheduledSourceNode.h>

#include <memory>
#include <cstddef>
#include <algorithm>
#include <string>

namespace audioapi {

class AudioBus;
class AudioParam;

class AudioBufferSourceNode : public AudioScheduledSourceNode {
 public:
  explicit AudioBufferSourceNode(BaseAudioContext *context, bool pitchCorrection);

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

  void start(double when, double offset, double duration = -1);

 protected:
  std::mutex &getBufferLock();
  void processNode(const std::shared_ptr<AudioBus>& processingBus, int framesToProcess) override;
  double getStopTime() const override;

 private:
  // Looping related properties
  bool loop_;
  double loopStart_;
  double loopEnd_;
  std::mutex bufferLock_;

  // pitch correction
  bool pitchCorrection_;

  // k-rate params
  std::shared_ptr<AudioParam> detuneParam_;
  std::shared_ptr<AudioParam> playbackRateParam_;

  std::shared_ptr<AudioBus> playbackRateBus_;

  // internal helper
  double vReadIndex_;

  // User provided buffer
  std::shared_ptr<AudioBuffer> buffer_;

  void processWithoutInterpolation(
      const std::shared_ptr<AudioBus>& processingBus,
      size_t startOffset,
      size_t offsetLength,
      float playbackRate);

  void processWithInterpolation(
      const std::shared_ptr<AudioBus>& processingBus,
      size_t startOffset,
      size_t offsetLength,
      float playbackRate);

  float getComputedPlaybackRateValue();

  double getVirtualStartFrame();
  double getVirtualEndFrame();
};

} // namespace audioapi
