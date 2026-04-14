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
#include <audioapi/core/types/BiquadFilterType.h>
#include <audioapi/utils/AudioBuffer.hpp>
#if RN_AUDIO_API_TEST
#include <gtest/gtest_prod.h>
#endif // RN_AUDIO_API_TEST

#include <memory>

namespace audioapi {

struct BiquadFilterOptions;

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
  std::shared_ptr<DSPAudioBuffer> processNode(
      const std::shared_ptr<DSPAudioBuffer> &processingBuffer,
      int framesToProcess) override;

 private:
  const std::shared_ptr<AudioParam> frequencyParam_;
  const std::shared_ptr<AudioParam> detuneParam_;
  const std::shared_ptr<AudioParam> QParam_;
  const std::shared_ptr<AudioParam> gainParam_;
  BiquadFilterType type_;

  // delayed samples, one per channel
  DSPAudioArray x1_;
  DSPAudioArray x2_;
  DSPAudioArray y1_;
  DSPAudioArray y2_;

  struct alignas(64) FilterCoefficients {
    double b0, b1, b2, a1, a2;
  };

  static FilterCoefficients getLowpassCoefficients(float frequency, float Q);
  static FilterCoefficients getHighpassCoefficients(float frequency, float Q);
  static FilterCoefficients getBandpassCoefficients(float frequency, float Q);
  static FilterCoefficients getLowshelfCoefficients(float frequency, float gain);
  static FilterCoefficients getHighshelfCoefficients(float frequency, float gain);
  static FilterCoefficients getPeakingCoefficients(float frequency, float Q, float gain);
  static FilterCoefficients getNotchCoefficients(float frequency, float Q);
  static FilterCoefficients getAllpassCoefficients(float frequency, float Q);
  static FilterCoefficients
  getNormalizedCoefficients(float b0, float b1, float b2, float a0, float a1, float a2);
  FilterCoefficients
  applyFilter(float frequency, float Q, float gain, float detune, BiquadFilterType type);
};

} // namespace audioapi
