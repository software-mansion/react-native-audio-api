#if defined(ANDROID)
#include "FFTFrame.h"
#include <fftw3.h>

namespace audioapi {
void FFTFrame::inverse(float *timeDomainData) {
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
} // namespace audioapi
#endif
