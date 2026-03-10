#pragma once

#include <audioapi/core/sources/AudioScheduledSourceNode.h>
#include <audioapi/libs/signalsmith-stretch/signalsmith-stretch.h>
#include <audioapi/utils/AudioBuffer.hpp>

#include <memory>

namespace audioapi {

class AudioParam;
struct BaseAudioBufferSourceOptions;

class AudioBufferBaseSourceNode : public AudioScheduledSourceNode {
 public:
  explicit AudioBufferBaseSourceNode(
      const std::shared_ptr<BaseAudioContext> &context,
      const BaseAudioBufferSourceOptions &options);

  /// @note JS Thread only
  void initStretch(const std::shared_ptr<signalsmith::stretch::SignalsmithStretch<float>> &stretch);

  [[nodiscard]] std::shared_ptr<AudioParam> getDetuneParam() const;
  [[nodiscard]] std::shared_ptr<AudioParam> getPlaybackRateParam() const;

  /// @note Audio Thread only
  void setOnPositionChangedCallbackId(uint64_t callbackId);

  /// @note Audio Thread only
  void setOnPositionChangedInterval(int interval);

  /// TODO remove and refactor
  [[nodiscard]] int getOnPositionChangedInterval() const;

  void unregisterOnPositionChangedCallback(uint64_t callbackId);

 protected:
  // pitch correction
  const bool pitchCorrection_;

  // pitch correction
  std::shared_ptr<signalsmith::stretch::SignalsmithStretch<float>> stretch_;
  std::shared_ptr<AudioBuffer> playbackRateBuffer_;

  // k-rate params
  const std::shared_ptr<AudioParam> detuneParam_;
  const std::shared_ptr<AudioParam> playbackRateParam_;

  // internal helper
  double vReadIndex_;

  uint64_t onPositionChangedCallbackId_ = 0; // 0 means no callback
  int onPositionChangedInterval_;
  int onPositionChangedTime_ = 0;

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
