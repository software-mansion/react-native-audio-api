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

#include <span>

namespace audioapi {

struct BiquadCoefficients {
  float b0;
  float b1;
  float b2;
  float a1;
  float a2;
};

void getFrequencyResponse(const BiquadCoefficients &coeffs, std::span<const float> frequency, std::span<float> mag_response, std::span<float> phase_response);

BiquadCoefficients normalizeCoefficients(float b0, float b1, float b2, float a0, float a1, float a2);

BiquadCoefficients calculateLowpassCoefficients(float cutoff, float Q);
BiquadCoefficients calculateHighpassCoefficients(float cutoff, float Q);
BiquadCoefficients calculateBandpassCoefficients(float frequency, float Q);
BiquadCoefficients calculateNotchCoefficients(float frequency, float Q);
BiquadCoefficients calculateAllpassCoefficients(float frequency, float Q);
BiquadCoefficients calculatePeakingCoefficients(float frequency, float Q, float db_gain);
BiquadCoefficients calculateLowshelfCoefficients(float frequency, float db_gain);
BiquadCoefficients calculateHighshelfCoefficients(float frequency, float db_gain);

} // namespace audioapi
