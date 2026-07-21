#pragma once

#include <audioapi/dsp/FFT.h>
#include <audioapi/utils/AudioArray.hpp>
#include <audioapi/utils/Macros.h>

#include <complex>
#include <memory>
#include <vector>

namespace audioapi::dsp {

/// @brief Shared windowed-FFT magnitude-spectrum analysis.
///
/// Owns the FFT scratch state (window, temp array, complex scratch, magnitude
/// output) so callers only need to supply a time-domain frame of `getFFTSize()`
/// samples and a smoothing constant.
///
/// @note Not thread-safe. `analyze()` and `setFFTSize()` must be called from a
/// single thread (or with external synchronization).
class SpectrumAnalyser {
 public:
  explicit SpectrumAnalyser(int fftSize);

  /// @brief Reallocates FFT scratch state for a new size. No-op if `fftSize`
  /// matches the current size. Resets magnitude data to zero.
  void setFFTSize(int fftSize);

  /// @brief Applies a Blackman window to `timeDomain`, runs the FFT, and
  /// exponentially smooths the resulting linear magnitude spectrum into
  /// `getMagnitudeData()`.
  /// @param timeDomain Must contain at least `getFFTSize()` samples.
  /// @param smoothingTimeConstant Exponential smoothing factor in `[0, 1]`.
  void analyze(const DSPAudioArray &timeDomain, float smoothingTimeConstant);

  [[nodiscard]] int getFFTSize() const {
    return fftSize_;
  }

  /// @brief Linear magnitude spectrum, length `getFFTSize() / 2`.
  [[nodiscard]] const DSPAudioArray &getMagnitudeData() const {
    return *magnitudeArray_;
  }

 private:
  void initializeWindowData(int fftSize);

  int fftSize_;

  std::unique_ptr<FFT> fft_;
  std::unique_ptr<DSPAudioArray> tempArray_;
  std::unique_ptr<DSPAudioArray> windowData_;
  std::unique_ptr<DSPAudioArray> magnitudeArray_;
  std::vector<std::complex<float>> complexData_;
};

} // namespace audioapi::dsp
