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

#include <audioapi/dsp/VectorMath.h>

#include <algorithm>
#include <cmath>
#include <utility>
#include <unordered_map>
#include <complex>

#if defined(HAVE_ACCELERATE)
#include <Accelerate/Accelerate.h>
#endif

#if defined(ANDROID)
#include <audioapi/android/libs/pffft.h>
#endif

namespace audioapi::fft {

class FFT {
 public:
  explicit FFT(int size);
  ~FFT();

  int getSize() const;

  void doFFT(float *data, float *realData, float *imaginaryData);
  void doInverseFFT(float *data, float *realData, float *imaginaryData);

 private:
  int size_;
  int log2Size_;

#if defined(HAVE_ACCELERATE)
    FFTSetup fftSetup_;
    DSPSplitComplex frame_;

    static FFTSetup getFFTSetupForSize(size_t log2FFTSize);
#endif

#if defined(ANDROID)
    PFFFT_Setup *pffftSetup_;
    float *work_;
#endif
};

} // namespace audioapi
