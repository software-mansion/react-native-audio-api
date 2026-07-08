#pragma once

#include <audioapi/compatibility/StableAPI.h>
#include <audioworklets/UIWorkletsRunner.h>

#include <atomic>
#include <cstddef>
#include <memory>
#include <vector>

namespace audioworklets {

/**
 * A pass-through analysis node that hands render-quantum audio snapshots to a
 * JavaScript worklet on the UI runtime so the UI can be animated from live
 * audio (e.g. amplitude/RMS visualizers).
 *
 * UI dispatches are capped at ~120 Hz (sample-rate aware) so the audio thread
 * does not flood the UI scheduler. Audio flows through unchanged.
 */
class WorkletNode : public audioapi::AudioNode {
 public:
  WorkletNode(
      const std::shared_ptr<audioapi::BaseAudioContext> &context,
      UIWorkletsRunner workletRunner);

 protected:
  void processNode(int framesToProcess) override;

 private:
  static constexpr float kMaxUiDispatchRateHz = 120.0f;

  void dispatchToUI(size_t frameCount, size_t channelCount);

  UIWorkletsRunner workletRunner_;

  /// @brief Fixed pool of per-channel UI snapshot buffers (MAX_CHANNEL_COUNT × render quantum).
  /// Allocated once in the constructor; never reallocated. Held in a shared_ptr so
  /// scheduled UI jobs keep the pool alive without per-quantum vector allocation.
  std::shared_ptr<std::vector<std::shared_ptr<audioapi::AudioArrayBuffer>>> snapshotBuffers_;

  /// @brief True while a UI-thread worklet invocation is still pending. Prevents
  /// the audio thread from flooding the UI scheduler; quanta are dropped instead.
  std::shared_ptr<std::atomic<bool>> busy_;

  /// @brief Audio-thread accumulator for sample-rate-aware UI dispatch throttling.
  size_t framesSinceLastDispatch_{0};

  /// @brief Minimum audio frames between UI worklet dispatches (~120 Hz at context sample rate).
  size_t minFramesBetweenDispatch_{0};
};

} // namespace audioworklets
