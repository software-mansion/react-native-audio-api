#pragma once

#include <audioapi/core/sources/AudioScheduledSourceNode.h>
#include <audioapi/core/utils/WsolaTimeStretcher.h>
#include <audioapi/utils/AudioBuffer.hpp>
#include <audioapi/utils/events/PositionChangedDispatcher.h>

#include <cstdint>
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
      size_t channelCount,
      float sampleRate,
      const std::shared_ptr<DSPAudioBuffer> &playbackRateBuffer);

  [[nodiscard]] std::shared_ptr<AudioParam> getDetuneParam() const;
  [[nodiscard]] std::shared_ptr<AudioParam> getPlaybackRateParam() const;

  /// @note Audio Thread only
  void setOnPositionChangedInterval(int interval);

  void assignOnPositionChangedCallbackId(uint64_t callbackId);

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
  WsolaTimeStretcher wsolaStretcher_;
  std::shared_ptr<DSPAudioBuffer> playbackRateBuffer_;

  // k-rate params
  const std::shared_ptr<AudioParam> detuneParam_;
  const std::shared_ptr<AudioParam> playbackRateParam_;

  PositionChangedDispatcher positionChanged_;

  void processWithPitchCorrection(
      const std::shared_ptr<DSPAudioBuffer> &processingBuffer,
      int framesToProcess);

  void processWithoutPitchCorrection(
      const std::shared_ptr<DSPAudioBuffer> &processingBuffer,
      int framesToProcess);

  float getComputedPlaybackRateValue(int framesToProcess, double time);
};

} // namespace audioapi
