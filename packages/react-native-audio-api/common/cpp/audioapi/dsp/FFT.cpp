#include <audioapi/dsp/FFT.h>

namespace audioapi::dsp {

FFT::FFT(int size) : size_(size) {
  pffftSetup_ = pffft_new_setup(size_, PFFFT_REAL);
  work_ = (float *)pffft_aligned_malloc(size_ * sizeof(float));
}

FFT::~FFT() {
  pffft_destroy_setup(pffftSetup_);
  pffft_aligned_free(work_);
}

void FFT::doFFT(float *in, std::vector<std::complex<float>> &out) {
  pffft_transform_ordered(
      pffftSetup_,
      in,
      reinterpret_cast<float *>(&out[0]),
      work_,
      PFFFT_FORWARD);

  dsp::multiplyByScalar(
      reinterpret_cast<float *>(&out[0]),
      0.5f,
      reinterpret_cast<float *>(&out[0]),
      size_ * 2);
}

void FFT::doInverseFFT(std::vector<std::complex<float>> &in, float *out) {
  pffft_transform_ordered(
      pffftSetup_,
      reinterpret_cast<float *>(&in[0]),
      out,
      work_,
      PFFFT_BACKWARD);

  dsp::multiplyByScalar(out, 1.0f / static_cast<float>(size_), out, size_);
}

} // namespace audioapi::dsp
