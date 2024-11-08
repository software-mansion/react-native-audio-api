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

#include "PeriodicWave.h"

// The number of bands per octave. Each octave will have this many entries in
// the wave tables.
constexpr unsigned NumberOfOctaveBands = 3;

constexpr float CentsPerRange = 1200.0f / NumberOfOctaveBands;

namespace audioapi {
PeriodicWave::PeriodicWave(int sampleRate) : sampleRate_(sampleRate) {
  numberOfRanges_ = lround(
      NumberOfOctaveBands * log2f(static_cast<float>(getPeriodicWaveSize())));
  auto nyquistFrequency = sampleRate_ / 2;
  lowestFundamentalFrequency_ = static_cast<float>(nyquistFrequency) /
      static_cast<float>(getMaxNumberOfPartials());
  rateScale_ = static_cast<float>(getPeriodicWaveSize()) /
      static_cast<float>(sampleRate_);
  waveTable_ = new float[getPeriodicWaveSize()];
}

PeriodicWave::PeriodicWave(int sampleRate, audioapi::OscillatorType type)
    : PeriodicWave(sampleRate) {
  // get waveTable for type
}

PeriodicWave::PeriodicWave(int sampleRate, float *real, float *imaginary)
    : PeriodicWave(sampleRate) {
  // get waveTable for real and imaginary
}

int PeriodicWave::getPeriodicWaveSize() const {
  if (sampleRate_ <= 24000) {
    return 2048;
  }

  if (sampleRate_ <= 88200) {
    return 4096;
  }

  return 16384;
}

void PeriodicWave::generateBasicWaveForm(OscillatorType type) {
  auto fftSize = getPeriodicWaveSize();
  /*
   * For real-valued time-domain signals, the FFT outputs a Hermitian symmetric sequence
   * (where the positive frequencies are the complex conjugate of the negative ones).
   * This symmetry implies that real signals have mirrored frequency components.
   * In such scenarios, all 'real' frequency information is contained in the first half of the transform,
   * and altering parts such as real and imag can finely shape which harmonic content is retained or discarded.
  */
  auto halfSize = fftSize / 2;

  auto *real = new float[halfSize];
  auto *imaginary = new float[halfSize];

  // Reset Direct Current (DC) component. First element of frequency domain representation - c0.
  // https://math24.net/complex-form-fourier-series.html
  real[0] = 0.0f;
  imaginary[0] = 0.0f;

  if (type == OscillatorType::SAWTOOTH) {
      real[0] = 1.0f;
  }

  for(int i = 1; i < halfSize; i++) {
      // All waveforms are odd functions with a positive slope at time 0.
      // Hence the coefficients for cos() are always 0.

      // Formulas for Fourier coefficients:
      // https://mathworld.wolfram.com/FourierSeries.html

      // Coefficient for sin()
      float b;

      auto piFactor = static_cast<float>(1.0f / (i * M_PI));

      switch(type) {
          case OscillatorType::SINE:
              b = (i == 1) ? 1.0f : 0.0f;
                break;

          case OscillatorType::SQUARE:
              // https://mathworld.wolfram.com/FourierSeriesSquareWave.html
              b = (i % 2 == 1) ? 4 * piFactor : 0.0f;
              break;
          case OscillatorType::SAWTOOTH:
              // https://mathworld.wolfram.com/FourierSeriesSawtoothWave.html
              b = - piFactor;
              break;
          case OscillatorType::TRIANGLE:
              // https://mathworld.wolfram.com/FourierSeriesTriangleWave.html
              if (i % 2 == 1) {
                  b = 8.0f * piFactor * piFactor * (i % 4 == 1 ? 1.0f : -1.0f);
              } else {
                  b = 0.0f;
              }
              break;
          case OscillatorType::CUSTOM:
             throw std::invalid_argument("Custom waveforms are not supported.");
      }

        real[i] = 0.0f;
        imaginary[i] = b;
  }

  // call creating waveTable from real and imaginary
  // createBandLimitedTables(real, imaginary, halfSize);
}

int PeriodicWave::getMaxNumberOfPartials() const {
  return getPeriodicWaveSize() / 2;
}
} // namespace audioapi
