#if defined(ANDROID)
#include "FFTFrame.h"
#include <kfr/dft.hpp>
#include <kfr/dsp.hpp>
#include <kfr/io.hpp>

namespace audioapi {
using namespace kfr;

void FFTFrame::inverse(float *timeDomainData) {
  univector<complex<float>> freqDomainData(size_ / 2);
  univector<float> timeData(size_);

    freqDomainData[0] = {0.0f, 0.0f};
  for (int i = 1; i < size_ / 2; i++) {
      freqDomainData[i] = {realData_[i], imaginaryData_[i]};
  }

  timeData = irealdft(freqDomainData) / (size_);

  for (int i = 0; i < size_; i++) {
      timeDomainData[i] = timeData[i];
  }
}
} // namespace audioapi
#endif
