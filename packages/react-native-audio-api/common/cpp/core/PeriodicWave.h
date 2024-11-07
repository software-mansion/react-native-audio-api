/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
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
 * 3.  Neither the name of Apple Inc. ("Apple") nor the names of
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

#include <cmath>
#include <memory>

#include "OscillatorType.h"

namespace audioapi {
class PeriodicWave {
 public:
  explicit PeriodicWave(int sampleRate, OscillatorType type);
  explicit PeriodicWave(int sampleRate, float *real, float *imaginary);

  int getPeriodicWaveSize() const;

 private:
  explicit PeriodicWave(int sampleRate);

  int getMaxNumberOfPartials() const;

  void generateBasicWaveForm(OscillatorType type);

  // determines the time resolution of the waveform.
  int sampleRate_;
  // determines number of frequency segments (or bands) the signal is divided.
  int numberOfRanges_;
  // the lowest frequency (in hertz) where playback will include all of the
  // partials.
  float lowestFundamentalFrequency_;
  // scaling factor used to adjust size of period of waveform to the sample
  // rate.
  float rateScale_;

  float *waveTable_;
  // float **bandLimitedTables_;
};
} // namespace audioapi
