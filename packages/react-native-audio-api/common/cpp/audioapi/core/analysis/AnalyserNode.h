#pragma once

#include <audioapi/core/AudioNode.h>
#include <audioapi/core/utils/Constants.h>
#include <audioapi/dsp/FFT.h>
#include <audioapi/utils/AudioArray.hpp>
#include <audioapi/utils/AudioBuffer.hpp>
#include <audioapi/utils/CircularArray.hpp>
#include <audioapi/utils/TripleBuffer.hpp>

#include <atomic>
#include <complex>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

namespace audioapi {

struct AnalyserOptions;

class AnalyserNode : public AudioNode {
 public:
  explicit AnalyserNode(
      const std::shared_ptr<BaseAudioContext> &context,
      const AnalyserOptions &options);

  /// @note JS Thread only
  float getMinDecibels() const {
    return minDecibels_;
  }

  /// @note JS Thread only
  float getMaxDecibels() const {
    return maxDecibels_;
  }

  /// @note JS Thread only
  float getSmoothingTimeConstant() const {
    return smoothingTimeConstant_;
  }

  int getFFTSize() const {
    return fftSize_.load(std::memory_order_acquire);
  }

  /// @note JS Thread only
  void setMinDecibels(float minDecibels) {
    minDecibels_ = minDecibels;
  }

  /// @note JS Thread only
  void setMaxDecibels(float maxDecibels) {
    maxDecibels_ = maxDecibels;
  }

  /// @note JS Thread only
  void setSmoothingTimeConstant(float smoothingTimeConstant) {
    smoothingTimeConstant_ = smoothingTimeConstant;
  }

  /// @note JS Thread only
  void setFFTSize(int fftSize);

  /// @note JS Thread only
  void getFloatFrequencyData(float *data, int length);

  /// @note JS Thread only
  void getByteFrequencyData(uint8_t *data, int length);

  /// @note JS Thread only
  void getFloatTimeDomainData(float *data, int length);

  /// @note JS Thread only
  void getByteTimeDomainData(uint8_t *data, int length);

 protected:
  std::shared_ptr<DSPAudioBuffer> processNode(
      const std::shared_ptr<DSPAudioBuffer> &processingBuffer,
      int framesToProcess) override;

 private:
  std::atomic<int> fftSize_;

  // Audio Thread data structures
  std::unique_ptr<CircularDSPAudioArray> inputArray_;
  std::unique_ptr<DSPAudioBuffer> downMixBuffer_;

  // JS Thread parameters
  float minDecibels_;
  float maxDecibels_;
  float smoothingTimeConstant_;

  // JS Thread data structures
  std::unique_ptr<dsp::FFT> fft_;
  std::unique_ptr<DSPAudioArray> tempArray_;
  std::unique_ptr<DSPAudioArray> windowData_;
  std::vector<std::complex<float>> complexData_;
  std::unique_ptr<DSPAudioArray> magnitudeArray_;

  struct AnalysisFrame {
    DSPAudioArray timeDomain;
    size_t sequenceNumber = 0;
    int fftSize = 0;

    explicit AnalysisFrame(size_t size) : timeDomain(size) {}

    AnalysisFrame(const AnalysisFrame &) = delete;
    AnalysisFrame &operator=(const AnalysisFrame &) = delete;
  };

  TripleBuffer<AnalysisFrame> analysisBuffer_{MAX_FFT_SIZE};
  size_t publishSequence_ = 0;      // audio thread only
  size_t lastAnalyzedSequence_ = 0; // JS thread only

  void doFFTAnalysis();

  void initializeWindowData(int fftSize);
};

} // namespace audioapi
