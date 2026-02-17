#pragma once

#include <audioapi/core/sources/AudioScheduledSourceNode.h>
#include <audioapi/libs/signalsmith-stretch/signalsmith-stretch.h>

#include <atomic>
#include <memory>
#include <mutex>

namespace audioapi {

class AudioBuffer;
class AudioParam;
struct BaseAudioBufferSourceOptions;

class AudioBufferBaseSourceNode : public AudioScheduledSourceNode {
 public:
  explicit AudioBufferBaseSourceNode(
      const std::shared_ptr<BaseAudioContext> &context,
      const BaseAudioBufferSourceOptions &options);

  [[nodiscard]] std::shared_ptr<AudioParam> getDetuneParam() const;
  [[nodiscard]] std::shared_ptr<AudioParam> getPlaybackRateParam() const;

  void setOnPositionChangedCallbackId(uint64_t callbackId);
  void setOnPositionChangedInterval(int interval);
  [[nodiscard]] int getOnPositionChangedInterval() const;
  [[nodiscard]] double getInputLatency() const;
  [[nodiscard]] double getOutputLatency() const;

 protected:
  // pitch correction
  bool pitchCorrection_;

  std::mutex bufferLock_;

  // pitch correction
  std::shared_ptr<signalsmith::stretch::SignalsmithStretch<float>> stretch_;
  std::shared_ptr<AudioBuffer> playbackRateBuffer_;

  // k-rate params
  std::shared_ptr<AudioParam> detuneParam_;
  std::shared_ptr<AudioParam> playbackRateParam_;

  // internal helper
  double vReadIndex_;

  std::atomic<uint64_t> onPositionChangedCallbackId_ = 0; // 0 means no callback
  int onPositionChangedInterval_;
  int onPositionChangedTime_ = 0;

  std::mutex &getBufferLock();
  virtual double getCurrentPosition() const = 0;

  void sendOnPositionChangedEvent();

  void processWithPitchCorrection(
      const std::shared_ptr<AudioBuffer> &processingBuffer,
      int framesToProcess);
  void processWithoutPitchCorrection(
      const std::shared_ptr<AudioBuffer> &processingBuffer,
      int framesToProcess);

  float getComputedPlaybackRateValue(int framesToProcess, double time);

  virtual void processWithoutInterpolation(
      const std::shared_ptr<AudioBuffer> &processingBuffer,
      size_t startOffset,
      size_t offsetLength,
      float playbackRate) = 0;

  virtual void processWithInterpolation(
      const std::shared_ptr<AudioBuffer> &processingBuffer,
      size_t startOffset,
      size_t offsetLength,
      float playbackRate) = 0;
};

} // namespace audioapi
