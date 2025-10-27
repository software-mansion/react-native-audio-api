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

  template<typename Allocator>
  void doFFT(float *in, std::vector<std::complex<float>, Allocator> &out) {
    pffft_transform_ordered(
        pffftSetup_,
        in,
        reinterpret_cast<float *>(&out[0]),
        work_,
        PFFFT_FORWARD);
    // this is a possible place for bugs and mistakes
    // due to pffft implementation and how it stores results
    // keep this information in mind
    // out[0].real = DC component - should be pure real
    // out[0].imag = Nyquist component - should be pure real
  }

  template<typename Allocator>
  void doInverseFFT(std::vector<std::complex<float>, Allocator> &in, float *out) {
    pffft_transform_ordered(
        pffftSetup_,
        reinterpret_cast<float *>(&in[0]),
        out,
        work_,
        PFFFT_BACKWARD);

    dsp::multiplyByScalar(out, 1.0f / static_cast<float>(size_), out, size_);
  }

 private:
  int size_;

  PFFFT_Setup *pffftSetup_;
  float *work_;
};

} // namespace audioapi::dsp
