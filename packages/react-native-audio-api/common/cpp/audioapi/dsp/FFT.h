#pragma once

#include <audioapi/dsp/VectorMath.h>
#include <audioapi/libs/pffft/pffft.h>

#include <algorithm>
#include <cmath>
#include <utility>
#include <complex>
#include <vector>

namespace audioapi::dsp {

class FFT {
 public:
  explicit FFT(int size);
  ~FFT();

  void doFFT(float *in, std::vector<std::complex<float>> &out);
  void doInverseFFT(std::vector<std::complex<float>> &in, float *out);

 private:
  int size_;

  PFFFT_Setup *pffftSetup_;
  float *work_;
};

} // namespace audioapi::dsp
