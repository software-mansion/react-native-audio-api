#pragma once

#include <audioapi/compatibility/StableAPI.h>
#include <audioworklets/UIWorkletsRunner.h>

#include <atomic>
#include <cstddef>
#include <memory>
#include <vector>

namespace audioworklets {

/**
 * A pass-through analysis node that hands each render quantum of audio data to a
 * JavaScript worklet running on the UI runtime, so the UI can be animated from
 * live audio (e.g. amplitude/RMS visualizers).
 *
 * Audio flows through unchanged. Buffer length and channel count match the
 * incoming quantum from the graph (`framesToProcess` and input channel count).
 */
class WorkletNode : public audioapi::AudioNode {
 public:
  WorkletNode(
      const std::shared_ptr<audioapi::BaseAudioContext> &context,
      UIWorkletsRunner workletRunner);

 protected:
  void processNode(int framesToProcess) override;

 private:
  void dispatchToUI(size_t frameCount, size_t channelCount);

  UIWorkletsRunner workletRunner_;

  /// @brief Fixed pool of per-channel UI snapshot buffers (MAX_CHANNEL_COUNT × render quantum).
  /// Allocated once in the constructor; never reallocated. Held in a shared_ptr so
  /// scheduled UI jobs keep the pool alive without per-quantum vector allocation.
  std::shared_ptr<std::vector<std::shared_ptr<audioapi::AudioArrayBuffer>>>
      snapshotBuffers_;

  /// @brief True while a UI-thread worklet invocation is still pending. Prevents
  /// the audio thread from flooding the UI scheduler; quanta are dropped instead.
  std::shared_ptr<std::atomic<bool>> busy_;
};

} // namespace audioworklets
