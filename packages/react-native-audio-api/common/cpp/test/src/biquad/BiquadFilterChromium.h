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

#include <numbers>
#include <span>

namespace audioapi {

constexpr double kPiDouble = std::numbers::pi_v<double>;

struct BiquadCoefficients {
  double b0;
  double b1;
  double b2;
  double a1;
  double a2;
};

void getFrequencyResponse(
const BiquadCoefficients &coeffs,
std::span<const float> frequency,
std::span<float> mag_response,
std::span<float> phase_response,
float nyquistFrequency);

BiquadCoefficients normalizeCoefficients(double b0, double b1, double b2, double a0, double a1, double a2);

BiquadCoefficients calculateLowpassCoefficients(double cutoff, double Q);
BiquadCoefficients calculateHighpassCoefficients(double cutoff, double Q);
BiquadCoefficients calculateBandpassCoefficients(double frequency, double Q);
BiquadCoefficients calculateNotchCoefficients(double frequency, double Q);
BiquadCoefficients calculateAllpassCoefficients(double frequency, double Q);
BiquadCoefficients calculatePeakingCoefficients(double frequency, double Q, double db_gain);
BiquadCoefficients calculateLowshelfCoefficients(double frequency, double db_gain);
BiquadCoefficients calculateHighshelfCoefficients(double frequency, double db_gain);

} // namespace audioapi
