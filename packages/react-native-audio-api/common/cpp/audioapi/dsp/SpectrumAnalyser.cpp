#include <audioapi/dsp/SpectrumAnalyser.h>

#include <cmath>
#include <memory>
#include <numbers>
#include <vector>

namespace audioapi::dsp {

SpectrumAnalyser::SpectrumAnalyser(int fftSize) : fftSize_(0) {
  setFFTSize(fftSize);
}

void SpectrumAnalyser::setFFTSize(int fftSize) {
  if (fftSize == fftSize_) {
    return;
  }

  fft_ = std::make_unique<FFT>(fftSize);
  complexData_ = std::vector<std::complex<float>>(fftSize);
  magnitudeArray_ = std::make_unique<DSPAudioArray>(fftSize / 2);
  tempArray_ = std::make_unique<DSPAudioArray>(fftSize);
  initializeWindowData(fftSize);
  fftSize_ = fftSize;
}

void SpectrumAnalyser::analyze(const DSPAudioArray &timeDomain, float smoothingTimeConstant) {
  tempArray_->copy(timeDomain, 0, 0, fftSize_);
  tempArray_->multiply(*windowData_, fftSize_);

  fft_->doFFT(*tempArray_, complexData_);

  // Zero out nquist component
  complexData_[0] = std::complex<float>(complexData_[0].real(), 0);

  const float magnitudeScale = 1.0f / static_cast<float>(fftSize_);
  auto magnitudeBufferData = magnitudeArray_->span();

  for (size_t i = 0; i < magnitudeArray_->getSize(); i++) {
    auto scalarMagnitude = std::abs(complexData_[i]) * magnitudeScale;
    magnitudeBufferData[i] = smoothingTimeConstant * magnitudeBufferData[i] +
        (1 - smoothingTimeConstant) * scalarMagnitude;
  }
}

void SpectrumAnalyser::initializeWindowData(int fftSize) {
  windowData_ = std::make_unique<DSPAudioArray>(static_cast<size_t>(fftSize));
  auto data = windowData_->span();
  const auto size = windowData_->getSize();

  const auto invSize = 1.0f / static_cast<float>(size);
  const auto alpha = 2.0f * std::numbers::pi_v<float> * invSize;

  for (size_t i = 0; i < size; ++i) {
    const auto phase = alpha * static_cast<float>(i);
    // 4*PI*x is just 2 * (2*PI*x)
    const auto window = 0.42f - 0.50f * std::cos(phase) + 0.08f * std::cos(2.0f * phase);
    data[i] = window;
  }
}

} // namespace audioapi::dsp
