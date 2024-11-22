#if defined(ANDROID)
#include "FFTFrame.h"
#include <kfr/dft.hpp>

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
//    auto* frame = (fftwf_complex*) fftwf_malloc(sizeof(fftwf_complex) * (size_ / 2 + 1));
//
//    // Create FFTW plan for real to complex forward transform
//    fftwf_plan plan_forward = fftwf_plan_dft_r2c_1d(size_, data, frame, FFTW_ESTIMATE);
//
//    // Execute the forward FFT
//    fftwf_execute(plan_forward);
//
//    // Extract real and imaginary parts from the FFTW output
//    for (int i = 0; i < size_ / 2 + 1; i++) {
//        realData_[i] = frame[i][0];
//        imaginaryData_[i] = frame[i][1];
//    }
//
//    // Cleanup FFTW resources
//    fftwf_destroy_plan(plan_forward);
//    fftwf_free(frame);
//
//    // Scale the FFT data by 0.5, just like with vDSP
//    VectorMath::multiplyByScalar(realData_, 0.5f, realData_, size_ / 2);
//    VectorMath::multiplyByScalar(imaginaryData_, 0.5f, imaginaryData_, size_ / 2);
}

void FFTFrame::inverse(float *data) {
//    // Allocate memory for the FFTW complex frequency-domain data
//    auto* frame = (fftwf_complex*) fftwf_malloc(sizeof(fftwf_complex) * (size_ / 2 + 1));
//
//    // Initialize input data for inverse transformation
//    for (int i = 0; i < size_ / 2 + 1; i++) {
//        frame[i][0] = realData_[i];
//        frame[i][1] = imaginaryData_[i];
//    }
//
//    // Create FFTW plan for complex to real inverse transform
//    fftwf_plan plan_backward = fftwf_plan_dft_c2r_1d(size_, frame, data, FFTW_ESTIMATE);
//
//    // Execute the inverse FFT
//    fftwf_execute(plan_backward);
//
//    // Normalize the result by dividing by the size
//    VectorMath::multiplyByScalar(data, 1.0f / static_cast<float>(size_) , data, size_);
//
//    // Cleanup FFTW resources
//    fftwf_destroy_plan(plan_backward);
//    fftwf_free(frame);
}
} // namespace audioapi
#endif
