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

#include <audioapi/core/types/OscillatorType.h>
#include <audioapi/dsp/FFT.h>

#include <algorithm>
#include <cmath>
#include <complex>
#include <memory>
#include <vector>

namespace audioapi {
class PeriodicWave {
 public:
  explicit PeriodicWave(float sampleRate, OscillatorType type, bool disableNormalization);
  explicit PeriodicWave(
      float sampleRate,
      const std::vector<std::complex<float>> &complexData,
      int length,
      bool disableNormalization);
  ~PeriodicWave();

  [[nodiscard]] int getPeriodicWaveSize() const;
  [[nodiscard]] float getScale() const;

  float getSample(float fundamentalFrequency, float phase, float phaseIncrement);

 private:
  explicit PeriodicWave(float sampleRate, bool disableNormalization);

  // Partial is any frequency component of a sound.
  // Both harmonics(fundamentalFrequency * k)  and overtones are partials.
  [[nodiscard]] int getMaxNumberOfPartials() const;

  // Returns the number of partials to keep for a given range.
  // Controlling the number of partials in each range allows for a more
  // efficient representation of the waveform and prevents aliasing.
  [[nodiscard]] int getNumberOfPartialsPerRange(int rangeIndex) const;

  // This function generates real and imaginary parts of the waveTable,
  // real and imaginary arrays represent the coefficients of the harmonic
  // components in the frequency domain, specifically as part of a complex
  // representation used by Fourier Transform methods to describe signals.
  void generateBasicWaveForm(OscillatorType type);

  // This function creates band-limited tables for the given real and
  // imaginary data. The tables are created for each range of the partials.
  // The higher frequencies are culled to band-limit the waveform.
  // For each range, the inverse FFT is performed to get the time domain
  // representation of the band-limited waveform.
  void createBandLimitedTables(const std::vector<std::complex<float>> &complexData, int size);

  // This function returns the interpolation factor between the lower and higher
  // range data and sets the lower and higher wave data for the given
  // fundamental frequency.
  float getWaveDataForFundamentalFrequency(
      float fundamentalFrequency,
      float *&lowerWaveData,
      float *&higherWaveData);

  // This function performs interpolation between the lower and higher range
  // data based on the interpolation factor and current buffer index. Type of
  // interpolation is determined by the phase increment. Returns the
  // interpolated sample.
  float doInterpolation(
      float bufferIndex,
      float phaseIncrement,
      float waveTableInterpolationFactor,
      const float *lowerWaveData,
      const float *higherWaveData) const;

  // determines the time resolution of the waveform.
  float sampleRate_;
  // determines number of frequency segments (or bands) the signal is divided.
  int numberOfRanges_;
  // the lowest frequency (in hertz) where playback will include all of the
  // partials.
  float lowestFundamentalFrequency_;
  // scaling factor used to adjust size of period of waveform to the sample
  // rate.
  float scale_;
  // array of band-limited waveforms.
  float **bandLimitedTables_;
  //
  std::unique_ptr<dsp::FFT> fft_;
  // if true, the waveTable is not normalized.
  bool disableNormalization_;
};
} // namespace audioapi
