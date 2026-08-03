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

/// @see https://webaudio.github.io/web-audio-api/#computedplaybackrate
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

  /// @brief Prefills WSOLA from the current @ref vReadIndex_ until the analysis queue is full.
  /// Call from @c start() after the read cursor is set. Advances @ref vReadIndex_ by the
  /// frames fed. This is the only WSOLA prefill path for buffer sources.
  /// @note Audio Thread only
  void primeWsolaInput();

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

  const std::shared_ptr<AudioParam> detuneParam_;
  const std::shared_ptr<AudioParam> playbackRateParam_;
  // Regular playback uses the spec-defined product; WSOLA needs playback rate
  // and detune independently to control speed and pitch.
  std::shared_ptr<CompositeAudioParam<combineComputedPlaybackRate>> computedPlaybackRateParam_;

  PositionChangedDispatcher positionChanged_;

  /// True after natural PCM EOF while WSOLA still has OLA tail to flush.
  /// Not armed for explicit stop(when) — that must cut without draining.
  bool wsolaDrainPending_{false};
  float wsolaEofDrainRate_{1.0f};

  /// Content accounting in output-time frames: PCM consumed / rate vs frames
  /// emitted. The EOF drain stops once emitted catches up with consumed, so the
  /// OLA tail cannot spill past the source's nominal end and double over a
  /// subsequently scheduled source.
  double wsolaExpectedOutputFrames_{0.0};
  double wsolaEmittedOutputFrames_{0.0};

  void processWithPitchCorrection(
      const std::shared_ptr<DSPAudioBuffer> &processingBuffer,
      int framesToProcess,
      double time);

  void processWithoutPitchCorrection(
      const std::shared_ptr<DSPAudioBuffer> &processingBuffer,
      int framesToProcess,
      double time);

  /// Flush remaining WSOLA output after PCM is exhausted (mirrors AudioFileSourceNode).
  void processWsolaDrain(
      const std::shared_ptr<DSPAudioBuffer> &processingBuffer,
      int framesToProcess);
};

} // namespace audioapi
