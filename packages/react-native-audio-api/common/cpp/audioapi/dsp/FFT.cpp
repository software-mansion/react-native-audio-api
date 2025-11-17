#include <audioapi/dsp/FFT.h>

namespace audioapi::dsp {

FFT::FFT(int size) : size_(size) {
  pffftSetup_ = pffft_new_setup(size_, PFFFT_REAL);
  work_ = reinterpret_cast<float *>(pffft_aligned_malloc(size_ * sizeof(float)));
}

FFT::~FFT() {
  pffft_destroy_setup(pffftSetup_);
  pffft_aligned_free(work_);
}

} // namespace audioapi::dsp
