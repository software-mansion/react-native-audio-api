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

  /// @note Audio Thread only
  void initStretch(
      const std::shared_ptr<signalsmith::stretch::SignalsmithStretch<float>> &stretch,
      const std::shared_ptr<DSPAudioBuffer> &playbackRateBuffer);

  [[nodiscard]] std::shared_ptr<AudioParam> getDetuneParam() const;
  [[nodiscard]] std::shared_ptr<AudioParam> getPlaybackRateParam() const;

  /// @note Audio Thread only
  void setOnPositionChangedCallbackId(uint64_t callbackId);

  /// @note Audio Thread only
  void setOnPositionChangedInterval(int interval);

  void unregisterOnPositionChangedCallback(uint64_t callbackId);

 protected:
  // internal helper
  double vReadIndex_;

  std::shared_ptr<DSPAudioBuffer> processNode(
      const std::shared_ptr<DSPAudioBuffer> &processingBuffer,
      int framesToProcess) final;

  virtual double getCurrentPosition() const = 0;

  virtual bool isEmpty() const = 0;

  virtual void runBufferProcessor(
      const std::shared_ptr<DSPAudioBuffer> &processingBuffer,
      size_t startOffset,
      size_t offsetLength,
      float playbackRate,
      bool interpolate) = 0;

 private:
  // pitch correction parameters
  // late init to avoid unnecessary allocation when pitch correction is not used.
  const bool pitchCorrection_;
  std::shared_ptr<signalsmith::stretch::SignalsmithStretch<float>> stretch_;
  std::shared_ptr<DSPAudioBuffer> playbackRateBuffer_;
  static constexpr float MAX_PLAYBACK_RATE = 3.0f;
  static constexpr float MIN_PLAYBACK_RATE = -3.0f;

  // k-rate params
  const std::shared_ptr<AudioParam> detuneParam_;
  const std::shared_ptr<AudioParam> playbackRateParam_;

  uint64_t onPositionChangedCallbackId_ = 0; // 0 means no callback
  int onPositionChangedIntervalInFrames_;
  int onPositionChangedTimeInFrames_ = 0;

  void sendOnPositionChangedEvent();

  void processWithPitchCorrection(
      const std::shared_ptr<DSPAudioBuffer> &processingBuffer,
      int framesToProcess);

  void processWithoutPitchCorrection(
      const std::shared_ptr<DSPAudioBuffer> &processingBuffer,
      int framesToProcess);

  float getComputedPlaybackRateValue(int framesToProcess, double time);
};

} // namespace audioapi
