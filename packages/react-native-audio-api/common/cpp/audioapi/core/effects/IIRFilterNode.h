/*
 * Copyright 2016 The Chromium Authors. All rights reserved.
 * Copyright (C) 2020 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include <audioapi/core/AudioNode.h>
#include <array>
#include <complex>
#include <cstddef>
#include <vector>

#include <audioapi/utils/AudioArray.hpp>
#include <audioapi/utils/AudioBuffer.hpp>
#include <memory>

namespace audioapi {

struct IIRFilterOptions;

class IIRFilterNode : public AudioNode {

 public:
  explicit IIRFilterNode(
      const std::shared_ptr<BaseAudioContext> &context,
      const IIRFilterOptions &options);

  /// @note Audio Thread only
  void getFrequencyResponse(
      const float *frequencyArray,
      float *magResponseOutput,
      float *phaseResponseOutput,
      size_t length) const;

 protected:
  void processNode(int framesToProcess) override;

  /// @brief IIR tail length, derived from the dominant pole magnitude of the
  /// (normalized) feedback polynomial. `r` is clamped to
  /// `1 - kPoleRadiusEpsilon` to keep the log bounded for poles near the
  /// unit circle, and the result is capped at `kMaxTailSeconds * sampleRate`
  /// for pathological coefficient sets.
  /// @note Audio Thread only.
  [[nodiscard]] int computeTailFrames() const override;

 private:
  static constexpr size_t BUFFER_LENGTH = 32;

  /// Envelope threshold at which the impulse response is considered inaudible (-80 dB).
  static constexpr double kTailEpsilon = 1e-4;

  /// Hard upper bound on the computed tail length (seconds).
  static constexpr float kMaxTailSeconds = 30.0f;

  /// Keeps `log(r)` bounded away from zero when the pole sits on or outside
  /// the unit circle.
  static constexpr double kPoleRadiusEpsilon = 1e-5;

  const AudioArray feedforward_;
  const AudioArray feedback_;

  AudioBuffer xBuffers_;
  AudioBuffer yBuffers_;
  std::array<size_t, BUFFER_LENGTH> bufferIndices_{};

  double dominantPoleRadius_ = 0.0;

  /// @brief Estimates `max|root|` of the normalized feedback polynomial
  /// `z^N + a_1 z^(N-1) + ... + a_N` using power iteration on its companion
  /// matrix. Returns 0 for degenerate (0-order) polynomials.
  /// @note Called once from the constructor.
  static double computeDominantPoleRadius(const AudioArray &feedback);

  static std::complex<float>
  evaluatePolynomial(const AudioArray &coefficients, std::complex<float> z, int order) {
    // Use Horner's method to evaluate the polynomial P(z) = sum(coef[k]*z^k, k, 0, order);
    std::complex<float> result = 0;
    for (int k = order; k >= 0; --k) {
      result = result * z + std::complex<float>(coefficients[k]);
    }
    return result;
  }

  static AudioArray createNormalizedArray(
      const std::vector<float> &inputVector,
      float scaleFactor) {
    AudioArray result(inputVector.data(), inputVector.size());
    if (scaleFactor != 1.0f && scaleFactor != 0.0f && result.getSize() > 0) {
      result.scale(1.0f / scaleFactor);
    }

    return result;
  }
};
} // namespace audioapi
