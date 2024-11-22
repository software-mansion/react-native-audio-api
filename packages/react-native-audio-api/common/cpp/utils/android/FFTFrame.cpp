#if defined(ANDROID)
#include "FFTFrame.h"
#include <kfr/dft.hpp>
#include <kfr/dsp.hpp>
#include <kfr/io.hpp>

namespace audioapi {

FFTFrame::FFTFrame(int size) {
  size_ = size;
  log2Size_ = static_cast<int>(log2(size));
  realData_ = new float[size];
  imaginaryData_ = new float[size];
}

FFTFrame::~FFTFrame() {
  delete[] realData_;
  delete[] imaginaryData_;
}

void FFTFrame::inverse(float *data) {
  using namespace kfr;

  univector<complex<float>> freq(size_ / 2);
  univector<float> time(size_);

  freq[0] = {0.0f, 0.0f};
  for (int i = 1; i < size_ / 2; i++) {
    freq[i] = {realData_[i], imaginaryData_[i]};
  }

  time = irealdft(freq) / (size_);

  for (int i = 0; i < size_; i++) {
    data[i] = time[i];
  }
}
} // namespace audioapi
#endif
