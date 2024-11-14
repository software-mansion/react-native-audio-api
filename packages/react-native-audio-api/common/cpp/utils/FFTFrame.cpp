#include "FFTFrame.h"

namespace audioapi {
    FFTFrame::FFTFrame(int size) {
        size_ = size;
        log2Size_ = static_cast<int>(log2(size));
        realData_ = new float[size];
        imaginaryData_ = new float[size];
        #if defined(HAVE_ACCELERATE)
                fftSetup_ = vDSP_create_fftsetup(log2Size_, FFT_RADIX2);
                frame_.realp = realData_;
                frame_.imagp = imaginaryData_;
        #endif
    }

#if defined(HAVE_ACCELERATE)
    void FFTFrame::forward(float *data) {
        vDSP_ctoz(reinterpret_cast<DSPComplex *>(data), 2, &frame_, 1, size_ / 2);
        vDSP_fft_zrip(fftSetup_, &frame_, 1, log2Size_, FFT_FORWARD);

        // Scale the FFT data, beacuse of
        // https://developer.apple.com/library/archive/documentation/Performance/Conceptual/vDSP_Programming_Guide/UsingFourierTransforms/UsingFourierTransforms.html#//apple_ref/doc/uid/TP40005147-CH3-15892
        VectorMath::multiplyByScalar(realData_, 0.5f, realData_, size_ / 2);
        VectorMath::multiplyByScalar(imaginaryData_, 0.5f, imaginaryData_, size_ /2);
    }

    void FFTFrame::inverse(float *data) {
        vDSP_fft_zrip(fftSetup_, &frame_, 1, log2Size_, FFT_INVERSE);
        vDSP_ztoc(&frame_, 1, (DSPComplex*)data, 2, size_ / 2);

        // Scale the FFT data, beacuse of
        // https://developer.apple.com/library/archive/documentation/Performance/Conceptual/vDSP_Programming_Guide/UsingFourierTransforms/UsingFourierTransforms.html#//apple_ref/doc/uid/TP40005147-CH3-15892
        VectorMath::multiplyByScalar(data, 1.0f / static_cast<float>(size_), data, size_);
    }
#else
  void FFTFrame::forward(float *data) {

  }

   void FFTFrame::inverse(float *data) {

   }
#endif
} // namespace audioapi
