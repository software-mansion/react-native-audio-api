#pragma once

#include <audioapi/compatibility/StableAPI.h>
#include <audioworklets/UIWorkletsRunner.h>

#include <atomic>
#include <cstddef>
#include <memory>

namespace audioworklets {

/**
 * A pass-through analysis node that hands buffered audio snapshots to a
 * JavaScript worklet on the UI runtime so the UI can be animated from live
 * audio (e.g. amplitude/RMS visualizers).
 *
 * Render-quantum frames are copied directly into snapshot buffers until
 * `bufferLength` is reached, then a snapshot is dispatched to the UI scheduler.
 * While the UI callback is running, incoming frames are skipped. Audio flows
 * through unchanged.
 */
class WorkletNode : public audioapi::AudioNode {
 public:
  WorkletNode(
      const std::shared_ptr<audioapi::BaseAudioContext> &context,
      UIWorkletsRunner workletRunner,
      size_t bufferLength);

  ~WorkletNode() override;

 protected:
  void processNode(int framesToProcess) override;

 private:
  void dispatchToUI(size_t channelCount);

  UIWorkletsRunner workletRunner_;
  size_t bufferLength_;
  size_t framesFilled_{0};

  /// @brief True while a UI-thread worklet invocation is still pending. Snapshot buffers
  /// are not filled until the callback completes.
  std::shared_ptr<std::atomic<bool>> busy_;
};

} // namespace audioworklets
