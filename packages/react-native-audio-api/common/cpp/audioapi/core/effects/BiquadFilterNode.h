/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include <audioapi/core/AudioNode.h>
#include <audioapi/core/AudioParam.h>
#include <audioapi/core/CompositeAudioParam.h>
#include <audioapi/core/types/BiquadFilterType.h>
#include <audioapi/utils/AudioBuffer.hpp>
#if RN_AUDIO_API_TEST
#include <gtest/gtest_prod.h>
#endif // RN_AUDIO_API_TEST

#include <array>
#include <cmath>
#include <memory>

namespace audioapi {

struct BiquadFilterOptions;

/// @brief computedFrequency(t) = frequency(t) * 2^(detune(t) / 1200).
/// https://webaudio.github.io/web-audio-api/#computedfrequency
inline float combineBiquadFrequency(float frequency, float detune) {
  return frequency * (detune == 0.0f ? 1.0f : std::pow(2.0f, detune / 1200.0f));
}

class BiquadFilterNode : public AudioNode {
#if RN_AUDIO_API_TEST
  friend class BiquadFilterTest;
  FRIEND_TEST(BiquadFilterTest, GetFrequencyResponse);
#endif // RN_AUDIO_API_TEST

 public:
  explicit BiquadFilterNode(
      const std::shared_ptr<BaseAudioContext> &context,
      const BiquadFilterOptions &options);

  void setType(BiquadFilterType);
  [[nodiscard]] std::shared_ptr<AudioParam> getFrequencyParam() const;
  [[nodiscard]] std::shared_ptr<AudioParam> getDetuneParam() const;
  [[nodiscard]] std::shared_ptr<AudioParam> getQParam() const;
  [[nodiscard]] std::shared_ptr<AudioParam> getGainParam() const;

  /// @note JS Thread only
  void getFrequencyResponse(
      const float *frequencyArray,
      float *magResponseOutput,
      float *phaseResponseOutput,
      size_t length,
      BiquadFilterType type);

 protected:
  void processNode(int framesToProcess) override;

  /// @brief IIR tail length, derived from the dominant pole magnitude of the
  /// most recently applied filter coefficients (`r = sqrt(|a2|)`). Returns
  /// the number of frames until the impulse response envelope decays below
  /// `kTailEpsilon`. `r` is clamped to `1 - kPoleRadiusEpsilon` to keep the
  /// log bounded for poles near the unit circle, and the final result is
  /// capped at `kMaxTailSeconds * sampleRate` for pathological high-Q cases.
  /// @note Audio Thread only.
  [[nodiscard]] int computeTailFrames() const override;

 private:
  /// Envelope threshold at which the impulse response is considered inaudible (-80 dB).
  static constexpr double kTailEpsilon = 1e-4;

  /// Hard upper bound on the computed tail length (seconds).
  static constexpr double kMaxTailSeconds = 30.0;

  /// Keeps `log(r)` bounded away from zero when the pole sits on the unit
  /// circle. `r` is clamped to `1 - kPoleRadiusEpsilon`, so the epsilon alone
  /// permits ≈ `log(kTailEpsilon) / log(1 - eps) ≈ 921 k frames` of tail at
  /// `eps = 1e-5` (≈ 20 s at 44.1 kHz) before `kMaxTailSeconds` takes over.
  static constexpr double kPoleRadiusEpsilon = 1e-5;

  const std::shared_ptr<AudioParam> frequencyParam_;
  const std::shared_ptr<AudioParam> detuneParam_;
  const std::shared_ptr<AudioParam> QParam_;
  const std::shared_ptr<AudioParam> gainParam_;
  // computedFrequency(t) = frequency(t) * 2^(detune(t) / 1200), clamped to [0, Nyquist].
  std::shared_ptr<CompositeAudioParam<combineBiquadFrequency>> computedFrequencyParam_;
  BiquadFilterType type_;

  /// Most recently applied `a2` coefficient, cached on the audio thread by
  /// `processNode`. Used by `computeTailFrames` to estimate decay length.
  /// `r = sqrt(|a2|)` is a conservative upper bound on the dominant pole
  /// magnitude for stable biquads.
  double lastA2_ = 0.0;

  // delayed samples, one per channel (double precision for filter state)
  std::array<double, MAX_CHANNEL_COUNT> x1_{};
  std::array<double, MAX_CHANNEL_COUNT> x2_{};
  std::array<double, MAX_CHANNEL_COUNT> y1_{};
  std::array<double, MAX_CHANNEL_COUNT> y2_{};

  struct alignas(64) FilterCoefficients {
    double b0, b1, b2, a1, a2;
  };

  static FilterCoefficients getLowpassCoefficients(double frequency, double Q);
  static FilterCoefficients getHighpassCoefficients(double frequency, double Q);
  static FilterCoefficients getBandpassCoefficients(double frequency, double Q);
  static FilterCoefficients getLowshelfCoefficients(double frequency, double gain);
  static FilterCoefficients getHighshelfCoefficients(double frequency, double gain);
  static FilterCoefficients getPeakingCoefficients(double frequency, double Q, double gain);
  static FilterCoefficients getNotchCoefficients(double frequency, double Q);
  static FilterCoefficients getAllpassCoefficients(double frequency, double Q);
  static FilterCoefficients
  getNormalizedCoefficients(double b0, double b1, double b2, double a0, double a1, double a2);
  /// @param computedFrequency Already-combined frequency in Hz (frequency * 2^(detune/1200)).
  FilterCoefficients
  applyFilter(double computedFrequency, double Q, double gain, BiquadFilterType type);
};

} // namespace audioapi
