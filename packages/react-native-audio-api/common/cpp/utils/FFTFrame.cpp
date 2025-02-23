#include "FFTFrame.h"

#if defined(HAVE_ACCELERATE)
#include <Accelerate/Accelerate.h>
#endif

#if defined(ANDROID)
#include <pffft.h>
#include <complex>
#endif

namespace audioapi {
#if defined(HAVE_ACCELERATE)
static std::unordered_map<size_t, FFTSetup> fftSetups_;

FFTFrame::FFTFrame(int size)
    : size_(size), log2Size_(static_cast<int>(log2(size))) {
  fftSetup_ = getFFTSetupForSize(log2Size_);
}

FFTFrame::~FFTFrame() {}

FFTSetup FFTFrame::getFFTSetupForSize(size_t log2FFTSize) {
  if (!fftSetups_.contains(log2FFTSize)) {
    fftSetups_.emplace(
        log2FFTSize, vDSP_create_fftsetup(log2FFTSize, FFT_RADIX2));
  }

  return fftSetups_.at(log2FFTSize);
}

void FFTFrame::doFFT(float *data, float *realData, float *imaginaryData) {
  frame_.realp = realData;
  frame_.imagp = imaginaryData;
  vDSP_ctoz(reinterpret_cast<DSPComplex *>(data), 2, &frame_, 1, size_ / 2);
  vDSP_fft_zrip(fftSetup_, &frame_, 1, log2Size_, FFT_FORWARD);

  VectorMath::multiplyByScalar(realData, 0.5f, realData, size_ / 2);
  VectorMath::multiplyByScalar(imaginaryData, 0.5f, imaginaryData, size_ / 2);
}

void FFTFrame::doInverseFFT(
    float *data,
    float *realData,
    float *imaginaryData) {
  frame_.realp = realData;
  frame_.imagp = imaginaryData;
  vDSP_fft_zrip(fftSetup_, &frame_, 1, log2Size_, FFT_INVERSE);
  vDSP_ztoc(&frame_, 1, reinterpret_cast<DSPComplex *>(data), 2, size_ / 2);

  // Scale the FFT data, beacuse of
  // https://developer.apple.com/library/archive/documentation/Performance/Conceptual/vDSP_Programming_Guide/UsingFourierTransforms/UsingFourierTransforms.html#//apple_ref/doc/uid/TP40005147-CH3-15892
  VectorMath::multiplyByScalar(
      data, 1.0f / static_cast<float>(size_), data, size_);
}

#elif defined(ANDROID)

FFTFrame::FFTFrame(int size)
    : size_(size), log2Size_(static_cast<int>(log2(size))) {
  pffftSetup_ = pffft_new_setup(size_, PFFFT_REAL);
  work_ = (float *)pffft_aligned_malloc(size_ * sizeof(float));
}

FFTFrame::~FFTFrame() {
  pffft_destroy_setup(pffftSetup_);
  pffft_aligned_free(work_);
}

void FFTFrame::doFFT(float *data, float *realData, float *imaginaryData) {
  std::vector<std::complex<float>> out(size_);
  pffft_transform_ordered(
      pffftSetup_,
      data,
      reinterpret_cast<float *>(&out[0]),
      work_,
      PFFFT_FORWARD);

  for (int i = 0; i < size_ / 2; ++i) {
    realData[i] = out[i].real();
    imaginaryData[i] = out[i].imag();
  }

  VectorMath::multiplyByScalar(realData, 0.5f, realData, size_ / 2);
  VectorMath::multiplyByScalar(imaginaryData, 0.5f, imaginaryData, size_ / 2);
}

void FFTFrame::doInverseFFT(
    float *data,
    float *realData,
    float *imaginaryData) {
  std::vector<std::complex<float>> out(size_ / 2);
  for (int i = 0; i < size_ / 2; i++) {
    out[i] = {realData[i], imaginaryData[i]};
  }

  pffft_transform_ordered(
      pffftSetup_,
      reinterpret_cast<float *>(&out[0]),
      data,
      work_,
      PFFFT_BACKWARD);

  VectorMath::multiplyByScalar(
      data, 1.0f / static_cast<float>(size_), data, size_);
}

#endif
} // namespace audioapi
