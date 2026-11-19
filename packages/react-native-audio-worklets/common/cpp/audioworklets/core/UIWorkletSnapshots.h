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
 * Pushed frames go into a ring that always holds the newest window, so a snapshot shows the
 * latest audio however large the pushed blocks are. A snapshot is due every `bufferLength`
 * frames (back-to-back windows in time domain, 50% overlap in frequency domain), and is sent on
 * the first push after it is due while no earlier one is still in flight; snapshots due while
 * the UI worklet runs are skipped. Shared by `WorkletNode` (fed from the audio thread) and
 * `RecorderWorklet` (fed from the recorder's worker).
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
  /// Waits for a full window of new frames before the next snapshot. @note Producer thread only.
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

  /// The newest `snapshotFrames()` pushed frames.
  std::unique_ptr<audioapi::CircularDSPAudioArray> recentFrames_;
  /// Frequency domain only: the window copied out of `recentFrames_` for the FFT.
  std::unique_ptr<audioapi::DSPAudioArray> fftInput_;
  std::unique_ptr<audioapi::dsp::SpectrumAnalyser> spectrumAnalyser_;
  /// Frames left until the next snapshot is due; negative while it is overdue, by that many.
  std::ptrdiff_t framesUntilSnapshot_;

  /// True while a UI-thread worklet invocation is pending; the snapshot memory is not written
  /// until it completes.
  std::shared_ptr<std::atomic<bool>> busy_;
};

} // namespace audioworklets
