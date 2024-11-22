#if defined(HAVE_ACCELERATE)
#include "FFTFrame.h"
#include "Accelerate/Accelerate.h"

namespace audioapi {
FFTFrame::FFTFrame(int size) {
  size_ = size;
  log2Size_ = static_cast<int>(log2(size));
  realData_ = new float[size];
  imaginaryData_ = new float[size];
  //  fftSetup_ = vDSP_create_fftsetup(log2Size_, FFT_RADIX2);
  //  frame_.realp = realData_;
  //  frame_.imagp = imaginaryData_;
}

FFTFrame::~FFTFrame() {
  delete[] realData_;
  delete[] imaginaryData_;
  //    vDSP_destroy_fftsetup(fftSetup_);
}

void FFTFrame::inverse(float *data) {
  FFTSetup fftSetup_ = vDSP_create_fftsetup(log2Size_, FFT_RADIX2);
  DSPSplitComplex frame_;
  frame_.realp = realData_;
  frame_.imagp = imaginaryData_;
  vDSP_fft_zrip(fftSetup_, &frame_, 1, log2Size_, FFT_INVERSE);
  vDSP_ztoc(&frame_, 1, reinterpret_cast<DSPComplex *>(data), 2, size_ / 2);

  // Scale the FFT data, beacuse of
  // https://developer.apple.com/library/archive/documentation/Performance/Conceptual/vDSP_Programming_Guide/UsingFourierTransforms/UsingFourierTransforms.html#//apple_ref/doc/uid/TP40005147-CH3-15892
  VectorMath::multiplyByScalar(
      data, 1.0f / static_cast<float>(size_), data, size_);

  vDSP_destroy_fftsetup(fftSetup_);
}
} // namespace audioapi
#endif
