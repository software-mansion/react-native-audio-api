#if defined(HAVE_ACCELERATE)
#include "FFTFrame.h"
#include "Accelerate/Accelerate.h"

namespace audioapi {

void FFTFrame::inverse(float *timeDomainData) {
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
} // namespace audioapi
#endif
