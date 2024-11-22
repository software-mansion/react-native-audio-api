#if defined(ANDROID)
#include "FFTFrame.h"
#include <kfr/dft.hpp>
#include <kfr/io.hpp>
#include <kfr/dsp.hpp>

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

void FFTFrame::forward(float *data) {

}

void FFTFrame::inverse(float *data) {
    kfr::dft_plan_real<float> plan(4096);
    kfr::univector<kfr::complex<float>, 4096> in;
    kfr::univector<float, 4096> out;

    for(int i = 0; i < 4096; i++) {
        in[i] = kfr::complex<float>(realData_[i], imaginaryData_[i]);
    }

    kfr::univector<kfr::u8> temp(plan.temp_size);
    plan.execute(out, in, temp);

    for(int i = 0; i < 4096; i++) {
        data[i] = out[i]/ static_cast<float>(size_);
    }
}
} // namespace audioapi
#endif
