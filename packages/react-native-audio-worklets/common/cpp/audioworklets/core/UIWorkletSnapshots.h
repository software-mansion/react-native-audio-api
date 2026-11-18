#pragma once

#include <audioapi/compatibility/StableAPI.h>
#include <audioworklets/UIWorkletsRunner.h>
#include <audioworklets/core/WorkletNodeDomain.h>
#include <audioworklets/types/NodeOptions.h>

#include <atomic>
#include <cstddef>
#include <memory>

namespace audioworklets {

/**
 * Turns a stream of mono audio into snapshots for a UI worklet: `bufferLength` PCM samples in
 * time domain, or `bufferLength` linear magnitude bins of a `bufferLength * 2`-point FFT in
 * frequency domain (the same analysis as `AnalyserNode`).
 *
 * One snapshot is in flight at a time: while the UI worklet still reads the last one, pushed
 * frames are skipped, and the rest of a block that completes a snapshot is dropped. Shared by
 * `WorkletNode` (fed from the audio thread) and `RecorderWorklet` (fed from the recorder's
 * worker).
 */
class UIWorkletSnapshots {
 public:
  UIWorkletSnapshots(
      UIWorkletsRunner runner,
      WorkletNodeDomain domain,
      const WorkletNodeOptions &options);
  ~UIWorkletSnapshots();
  DELETE_COPY_AND_MOVE(UIWorkletSnapshots);

  /// @note Producer thread only, never concurrently with reset().
  void push(const float *mono, size_t numFrames);
  /// Drops the partly filled snapshot. @note Producer thread only.
  void reset();
  /// The rate of the frames pushed from now on, handed to the worklet with each snapshot.
  /// @note Producer thread only, or before it starts pushing.
  void setSampleRate(float sampleRate) {
    sampleRate_ = sampleRate;
  }

  /// Stops scheduling UI work; pushes become no-ops. Any thread; safe to call more than once.
  void deactivate();

  [[nodiscard]] WorkletNodeDomain getDomain() const {
    return domain_;
  }

  [[nodiscard]] size_t getBufferLength() const {
    return bufferLength_;
  }

  [[nodiscard]] float getSmoothingTimeConstant() const {
    return smoothingTimeConstant_.load(std::memory_order_acquire);
  }

  void setSmoothingTimeConstant(float smoothingTimeConstant) {
    smoothingTimeConstant_.store(smoothingTimeConstant, std::memory_order_release);
  }

 private:
  /// Frames one snapshot is computed from.
  [[nodiscard]] size_t snapshotFrames() const;
  void dispatchToUI();

  UIWorkletsRunner runner_;
  const WorkletNodeDomain domain_;
  const size_t bufferLength_;
  std::atomic<float> smoothingTimeConstant_;
  float sampleRate_ = 0.0F;

  /// Time domain: the next snapshot's samples. Frequency domain: the FFT input.
  std::unique_ptr<audioapi::DSPAudioArray> accumulator_;
  std::unique_ptr<audioapi::dsp::SpectrumAnalyser> spectrumAnalyser_;
  size_t framesFilled_{0};

  /// True while a UI-thread worklet invocation is pending; the snapshot memory is not written
  /// until it completes.
  std::shared_ptr<std::atomic<bool>> busy_;
};

} // namespace audioworklets
