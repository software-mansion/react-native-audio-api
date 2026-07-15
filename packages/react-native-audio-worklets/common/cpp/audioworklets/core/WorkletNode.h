#pragma once

#include <audioapi/compatibility/StableAPI.h>
#include <audioworklets/UIWorkletsRunner.h>
#include <audioworklets/core/WorkletNodeDomain.h>

#include <atomic>
#include <complex>
#include <cstddef>
#include <memory>
#include <vector>

namespace audioworklets {

struct WorkletNodeOptions {
  size_t bufferLength = 1024;
  float smoothingTimeConstant = audioapi::AnalyserOptions::kDefaultSmoothingTimeConstant;
};

/**
 * A pass-through analysis node that hands buffered audio snapshots to a
 * JavaScript worklet on the UI runtime so the UI can be animated from live
 * audio (e.g. amplitude/RMS or spectrum visualizers).
 *
 * `bufferLength` is the snapshot size passed to the UI callback in both modes:
 * time-domain PCM samples, or frequency-domain linear magnitude bins. In
 * frequency domain the internal FFT size is `bufferLength * 2` (same analysis
 * path as `AnalyserNode`).
 */
class WorkletNode : public audioapi::AudioNode {
 public:
  WorkletNode(
      const std::shared_ptr<audioapi::BaseAudioContext> &context,
      UIWorkletsRunner workletRunner,
      WorkletNodeDomain domain,
      const WorkletNodeOptions &options);

  ~WorkletNode() override;

  DELETE_COPY_AND_MOVE(WorkletNode);

  [[nodiscard]] WorkletNodeDomain getDomain() const {
    return domain_;
  }

  /// @note JS Thread only.
  [[nodiscard]] size_t getBufferLength() const {
    return bufferLength_;
  }

  /// @note JS Thread only.
  [[nodiscard]] float getSmoothingTimeConstant() const {
    return smoothingTimeConstant_.load(std::memory_order_acquire);
  }

  /// @note JS Thread only.
  void setSmoothingTimeConstant(float smoothingTimeConstant);

 protected:
  void processNode(int framesToProcess) override;

  void processInputs(const std::vector<const audioapi::DSPAudioBuffer *> &inputs, int numFrames)
      override;

 private:
  void dispatchToUI();
  void processTimeDomain(int framesToProcess);
  void processFrequencyDomain(int framesToProcess);
  void doFFTAnalysis();
  void initializeWindowData(int fftSize);
  void initializeFrequencyDomain(int fftSize);

  static size_t fftSizeForBufferLength(size_t bufferLength);

  WorkletNodeDomain domain_;
  const size_t bufferLength_;
  std::atomic<float> smoothingTimeConstant_{
      audioapi::AnalyserOptions::kDefaultSmoothingTimeConstant};

  std::unique_ptr<audioapi::DSPAudioBuffer> downMixBuffer_;
  std::unique_ptr<audioapi::DSPAudioArray> timeDomainAccum_;

  std::unique_ptr<audioapi::dsp::FFT> fft_;
  std::unique_ptr<audioapi::DSPAudioArray> frequencyTimeDomainAccum_;
  std::unique_ptr<audioapi::DSPAudioArray> tempArray_;
  std::unique_ptr<audioapi::DSPAudioArray> windowData_;
  std::unique_ptr<audioapi::DSPAudioArray> magnitudeArray_;
  std::vector<std::complex<float>> complexData_;

  UIWorkletsRunner workletRunner_;
  size_t framesFilled_{0};

  /// @brief True while a UI-thread worklet invocation is still pending. Snapshot buffers
  /// are not filled until the callback completes.
  std::shared_ptr<std::atomic<bool>> busy_;
};

} // namespace audioworklets
