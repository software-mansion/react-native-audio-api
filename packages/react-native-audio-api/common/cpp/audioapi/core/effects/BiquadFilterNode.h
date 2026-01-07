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
#if RN_AUDIO_API_TEST
#include <gtest/gtest_prod.h>
#endif // RN_AUDIO_API_TEST

#include <algorithm>
#include <cmath>
#include <complex>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace audioapi {

class AudioBus;

class BiquadFilterNode : public AudioNode {
#if RN_AUDIO_API_TEST
  friend class BiquadFilterTest;
  FRIEND_TEST(BiquadFilterTest, GetFrequencyResponse);
#endif // RN_AUDIO_API_TEST

 public:
  explicit BiquadFilterNode(std::shared_ptr<BaseAudioContext> context);

  [[nodiscard]] std::string getType();
  void setType(const std::string &type);
  [[nodiscard]] std::shared_ptr<AudioParam> getFrequencyParam() const;
  [[nodiscard]] std::shared_ptr<AudioParam> getDetuneParam() const;
  [[nodiscard]] std::shared_ptr<AudioParam> getQParam() const;
  [[nodiscard]] std::shared_ptr<AudioParam> getGainParam() const;
  void getFrequencyResponse(
      const float *frequencyArray,
      float *magResponseOutput,
      float *phaseResponseOutput,
      size_t length);

 protected:
  std::shared_ptr<AudioBus> processNode(
      const std::shared_ptr<AudioBus> &processingBus,
      int framesToProcess) override;

 private:
  std::shared_ptr<AudioParam> frequencyParam_;
  std::shared_ptr<AudioParam> detuneParam_;
  std::shared_ptr<AudioParam> QParam_;
  std::shared_ptr<AudioParam> gainParam_;
  audioapi::BiquadFilterType type_;

  // delayed samples, one per channel
  std::vector<float> x1_;
  std::vector<float> x2_;
  std::vector<float> y1_;
  std::vector<float> y2_;

  // coefficients
  float b0_ = 1.0;
  float b1_ = 0;
  float b2_ = 0;
  float a1_ = 0;
  float a2_ = 0;

  static BiquadFilterType fromString(const std::string &type) {
    std::string lowerType = type;
    std::transform(lowerType.begin(), lowerType.end(), lowerType.begin(), ::tolower);

    if (lowerType == "lowpass")
      return BiquadFilterType::LOWPASS;
    if (lowerType == "highpass")
      return BiquadFilterType::HIGHPASS;
    if (lowerType == "bandpass")
      return BiquadFilterType::BANDPASS;
    if (lowerType == "lowshelf")
      return BiquadFilterType::LOWSHELF;
    if (lowerType == "highshelf")
      return BiquadFilterType::HIGHSHELF;
    if (lowerType == "peaking")
      return BiquadFilterType::PEAKING;
    if (lowerType == "notch")
      return BiquadFilterType::NOTCH;
    if (lowerType == "allpass")
      return BiquadFilterType::ALLPASS;

    throw std::invalid_argument("Invalid filter type: " + type);
  }

  static std::string toString(BiquadFilterType type) {
    switch (type) {
      case BiquadFilterType::LOWPASS:
        return "lowpass";
      case BiquadFilterType::HIGHPASS:
        return "highpass";
      case BiquadFilterType::BANDPASS:
        return "bandpass";
      case BiquadFilterType::LOWSHELF:
        return "lowshelf";
      case BiquadFilterType::HIGHSHELF:
        return "highshelf";
      case BiquadFilterType::PEAKING:
        return "peaking";
      case BiquadFilterType::NOTCH:
        return "notch";
      case BiquadFilterType::ALLPASS:
        return "allpass";
      default:
        throw std::invalid_argument("Unknown filter type");
    }
  }

  void setNormalizedCoefficients(float b0, float b1, float b2, float a0, float a1, float a2);
  void setLowpassCoefficients(float frequency, float Q);
  void setHighpassCoefficients(float frequency, float Q);
  void setBandpassCoefficients(float frequency, float Q);
  void setLowshelfCoefficients(float frequency, float gain);
  void setHighshelfCoefficients(float frequency, float gain);
  void setPeakingCoefficients(float frequency, float Q, float gain);
  void setNotchCoefficients(float frequency, float Q);
  void setAllpassCoefficients(float frequency, float Q);
  void applyFilter();
};

} // namespace audioapi
