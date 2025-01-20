#include "FFTFrame.h"

#if defined(HAVE_ACCELERATE)
#include <Accelerate/Accelerate.h>
#endif

#if defined(ANDROID)
#include <fftw3.h>
#endif

namespace audioapi {
#if defined(HAVE_ACCELERATE)
static std::unordered_map<size_t, FFTSetup> fftSetups_;

FFTFrame::FFTFrame(int size)
    : size_(size),
      log2Size_(static_cast<int>(log2(size))),
      realData_(new float[size]),
      imaginaryData_(new float[size]) {
  fftSetup_ = getFFTSetupForSize(log2Size_);
  frame_.realp = realData_;
  frame_.imagp = imaginaryData_;
}

FFTFrame::~FFTFrame() {
  delete[] realData_;
  delete[] imaginaryData_;
}

FFTSetup FFTFrame::getFFTSetupForSize(size_t log2FFTSize) {
  if (!fftSetups_.contains(log2FFTSize)) {
    fftSetups_.emplace(
        log2FFTSize, vDSP_create_fftsetup(log2FFTSize, FFT_RADIX2));
  }

  return fftSetups_.at(log2FFTSize);
}

void FFTFrame::doFFT(float *data) {
  vDSP_ctoz(reinterpret_cast<DSPComplex *>(data), 2, &frame_, 1, size_ / 2);
  vDSP_fft_zrip(fftSetup_, &frame_, 1, log2Size_, FFT_FORWARD);

  VectorMath::multiplyByScalar(realData_, 0.5f, realData_, size_ / 2);
  VectorMath::multiplyByScalar(imaginaryData_, 0.5f, imaginaryData_, size_ / 2);
}

void FFTFrame::doInverseFFT(float *data) {
  vDSP_fft_zrip(fftSetup_, &frame_, 1, log2Size_, FFT_INVERSE);
  vDSP_ztoc(&frame_, 1, reinterpret_cast<DSPComplex *>(data), 2, size_ / 2);

  // Scale the FFT data, beacuse of
  // https://developer.apple.com/library/archive/documentation/Performance/Conceptual/vDSP_Programming_Guide/UsingFourierTransforms/UsingFourierTransforms.html#//apple_ref/doc/uid/TP40005147-CH3-15892
  VectorMath::multiplyByScalar(
      data, 1.0f / static_cast<float>(size_), data, size_);
}

#elif defined(ANDROID)

FFTFrame::FFTFrame(int size)
    : size_(size),
      log2Size_(static_cast<int>(log2(size))),
      realData_(new float[size]),
      imaginaryData_(new float[size]) {
  frame_ = fftwf_alloc_complex(size / 2);
}

FFTFrame::~FFTFrame() {
  delete[] realData_;
  delete[] imaginaryData_;
  fftwf_free(frame_);
}

void FFTFrame::doFFT(float *data) {
  auto plan = fftwf_plan_dft_r2c_1d(size_, data, frame_, FFTW_ESTIMATE);
  fftwf_execute(plan);
  fftwf_destroy_plan(plan);

  for (int i = 0; i < size_ / 2; ++i) {
    realData_[i] = frame_[i][0];
    imaginaryData_[i] = frame_[i][1];
  }

  VectorMath::multiplyByScalar(realData_, 0.5f, realData_, size_ / 2);
  VectorMath::multiplyByScalar(imaginaryData_, 0.5f, imaginaryData_, size_ / 2);
}

void FFTFrame::doInverseFFT(float *data) {
  for (int i = 0; i < size_ / 2; i++) {
    frame_[i][0] = realData_[i];
    frame_[i][1] = imaginaryData_[i];
  }

  auto plan = fftwf_plan_dft_c2r_1d(size_, frame_, data, FFTW_ESTIMATE);
  fftwf_execute(plan);
  fftwf_destroy_plan(plan);

  VectorMath::multiplyByScalar(
      data, 1.0f / static_cast<float>(size_), data, size_);
}

#endif
} // namespace audioapi
