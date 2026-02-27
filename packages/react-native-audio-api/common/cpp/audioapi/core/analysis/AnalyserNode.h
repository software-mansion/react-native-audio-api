#pragma once

#include <audioapi/core/AudioNode.h>
#include <audioapi/core/utils/Constants.h>
#include <audioapi/dsp/FFT.h>
#include <audioapi/dsp/Windows.hpp>
#include <audioapi/utils/AudioArray.h>
#include <audioapi/utils/TripleBuffer.hpp>

#include <atomic>
#include <complex>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

namespace audioapi {

class AudioBuffer;
class CircularAudioArray;
struct AnalyserOptions;

class AnalyserNode : public AudioNode {
 public:
  explicit AnalyserNode(
      const std::shared_ptr<BaseAudioContext> &context,
      const AnalyserOptions &options);

  void setFFTSize(int fftSize);
  void setMinDecibels(float minDecibels);
  void setMaxDecibels(float maxDecibels);
  void setSmoothingTimeConstant(float smoothingTimeConstant);

  void getFloatFrequencyData(float *data, int length);
  void getByteFrequencyData(uint8_t *data, int length);
  void getFloatTimeDomainData(float *data, int length);
  void getByteTimeDomainData(uint8_t *data, int length);

 protected:
  std::shared_ptr<AudioBuffer> processNode(
      const std::shared_ptr<AudioBuffer> &processingBuffer,
      int framesToProcess) override;

 private:
  std::atomic<int> fftSize_;
  std::atomic<float> minDecibels_;
  std::atomic<float> maxDecibels_;
  std::atomic<float> smoothingTimeConstant_;

  // Audio Thread data structures
  std::unique_ptr<CircularAudioArray> inputArray_;
  std::unique_ptr<AudioBuffer> downMixBuffer_;

  // JS Thread data structures
  std::unique_ptr<dsp::FFT> fft_;
  std::unique_ptr<AudioArray> tempArray_;
  std::unique_ptr<AudioArray> windowData_;
  std::vector<std::complex<float>> complexData_;
  std::unique_ptr<AudioArray> magnitudeArray_;

  struct AnalysisFrame {
    AudioArray timeDomain;
    size_t sequenceNumber = 0;

    AnalysisFrame() : timeDomain(MAX_FFT_SIZE) {}
  };

  TripleBuffer<AnalysisFrame> analysisBuffer_;
  size_t publishSequence_ = 0;      // audio thread only
  size_t lastAnalyzedSequence_ = 0; // JS thread only

  void doFFTAnalysis();
};

} // namespace audioapi
