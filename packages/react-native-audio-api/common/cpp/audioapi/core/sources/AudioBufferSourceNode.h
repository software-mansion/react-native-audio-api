#pragma once

#include <audioapi/core/sources/AudioBuffer.h>
#include <audioapi/core/sources/AudioScheduledSourceNode.h>

#include <memory>
#include <cstddef>
#include <algorithm>
#include <cassert>

namespace audioapi {

class AudioBus;
class AudioParam;

class AudioBufferSourceNode : public AudioScheduledSourceNode {
 public:
  explicit AudioBufferSourceNode(BaseAudioContext *context);
  ~AudioBufferSourceNode();

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

 private:
  // Looping related properties
  bool loop_;
  double loopStart_;
  double loopEnd_;
  std::mutex bufferLock_;

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
      const std::shared_ptr<AudioBus>& processingBus,
      size_t startOffset,
      size_t offsetLength,
      float playbackRate);

  void processWithInterpolation(
      const std::shared_ptr<AudioBus>& processingBus,
      size_t startOffset,
      size_t offsetLength,
      float playbackRate);
};

} // namespace audioapi
