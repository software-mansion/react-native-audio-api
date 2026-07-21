#pragma once

#include <audioapi/core/CompositeAudioParam.h>
#include <audioapi/core/sources/AudioScheduledSourceNode.h>
#include <audioapi/core/utils/Constants.h>
#include <audioapi/dsp/WsolaTimeStretcher.h>
#include <audioapi/utils/AudioBuffer.hpp>
#include <audioapi/utils/events/PositionChangedDispatcher.h>

#include <cmath>
#include <cstdint>
#include <memory>

namespace audioapi {

class AudioParam;
struct BaseAudioBufferSourceOptions;

/// @brief computedPlaybackRate(t) = playbackRate(t) * 2^(detune(t) / 1200).
/// https://webaudio.github.io/web-audio-api/#dom-audiobuffersourcenode-playbackrate
/// Pure spec formula — WSOLA's ±MAX_PLAYBACK_RATE limit is applied only in the
/// pitch-correction path, which feeds that stretcher.
inline float combineComputedPlaybackRate(float playbackRate, float detune) {
  return playbackRate * std::pow(2.0f, detune / static_cast<float>(OCTAVE_RANGE));
}

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

  void processNode(int framesToProcess) final;

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
  // computedPlaybackRate(t) = playbackRate(t) * 2^(detune(t) / 1200).
  // The simple children above are kept so the pitch-correction path can read
  // the constituents (rate, pitchFactor) separately.
  std::shared_ptr<CompositeAudioParam<combineComputedPlaybackRate>> computedPlaybackRateParam_;

  PositionChangedDispatcher positionChanged_;

  void processWithPitchCorrection(
      const std::shared_ptr<DSPAudioBuffer> &processingBuffer,
      int framesToProcess,
      double time);

  void processWithoutPitchCorrection(
      const std::shared_ptr<DSPAudioBuffer> &processingBuffer,
      int framesToProcess,
      double time);
};

} // namespace audioapi
