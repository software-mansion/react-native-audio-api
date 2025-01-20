#include "FFTFrame.h"

#if defined(HAVE_ACCELERATE)
#include <Accelerate/Accelerate.h>
#endif

#if defined(ANDROID)
#include <fftw3.h>
#endif

namespace audioapi {
#if defined(HAVE_ACCELERATE)

void FFTFrame::doFFT(float *data) {
  FFTSetup fftSetup_ = vDSP_create_fftsetup(log2Size_, FFT_RADIX2);
  DSPSplitComplex freqDomainData;
  freqDomainData.realp = realData_;
  freqDomainData.imagp = imaginaryData_;

  vDSP_ctoz(
      reinterpret_cast<DSPComplex *>(data), 2, &freqDomainData, 1, size_ / 2);
  vDSP_fft_zrip(fftSetup_, &freqDomainData, 1, log2Size_, FFT_FORWARD);

  VectorMath::multiplyByScalar(realData_, 0.5f, realData_, size_ / 2);
  VectorMath::multiplyByScalar(imaginaryData_, 0.5f, imaginaryData_, size_ / 2);

  vDSP_destroy_fftsetup(fftSetup_);
}

void FFTFrame::doInverseFFT(float *timeDomainData) {
  FFTSetup fftSetup_ = vDSP_create_fftsetup(log2Size_, FFT_RADIX2);
  DSPSplitComplex freqDomainData;
  freqDomainData.realp = realData_;
  freqDomainData.imagp = imaginaryData_;

  vDSP_fft_zrip(fftSetup_, &freqDomainData, 1, log2Size_, FFT_INVERSE);
  vDSP_ztoc(
      &freqDomainData,
      1,
      reinterpret_cast<DSPComplex *>(timeDomainData),
      2,
      size_ / 2);

  // Scale the FFT data, beacuse of
  // https://developer.apple.com/library/archive/documentation/Performance/Conceptual/vDSP_Programming_Guide/UsingFourierTransforms/UsingFourierTransforms.html#//apple_ref/doc/uid/TP40005147-CH3-15892
  VectorMath::multiplyByScalar(
      timeDomainData, 1.0f / static_cast<float>(size_), timeDomainData, size_);

  vDSP_destroy_fftsetup(fftSetup_);
}

#elif defined(ANDROID)

void FFTFrame::doFFT(float *data) {
  fftwf_complex *freqDomainData = fftwf_alloc_complex(size_ / 2);

  auto plan = fftwf_plan_dft_r2c_1d(size_, data, freqDomainData, FFTW_ESTIMATE);
  fftwf_execute(plan);

  for (int i = 0; i < size_ / 2; ++i) {
    realData_[i] = freqDomainData[i][0];
    imaginaryData_[i] = freqDomainData[i][1];
  }

  fftwf_destroy_plan(plan);
  fftwf_free(freqDomainData);

  VectorMath::multiplyByScalar(realData_, 0.5f, realData_, size_ / 2);
  VectorMath::multiplyByScalar(imaginaryData_, 0.5f, imaginaryData_, size_ / 2);
}

void FFTFrame::doInverseFFT(float *timeDomainData) {
  fftwf_complex *freqDomainData = fftwf_alloc_complex(size_ / 2);
  for (int i = 0; i < size_ / 2; i++) {
    freqDomainData[i][0] = realData_[i];
    freqDomainData[i][1] = imaginaryData_[i];
  }

  auto plan = fftwf_plan_dft_c2r_1d(
      size_, freqDomainData, timeDomainData, FFTW_ESTIMATE);
  fftwf_execute(plan);
  fftwf_destroy_plan(plan);
  fftwf_free(freqDomainData);

  VectorMath::multiplyByScalar(
      timeDomainData, 1.0f / static_cast<float>(size_), timeDomainData, size_);
}

#endif
} // namespace audioapi
