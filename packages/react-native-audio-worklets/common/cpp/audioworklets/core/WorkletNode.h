#pragma once

#include <audioapi/compatibility/StableAPI.h>
#include <audioworklets/UIWorkletsRunner.h>

#include <atomic>
#include <cstddef>
#include <memory>
#include <vector>

namespace audioworklets {

/**
 * A pass-through analysis node that hands buffered audio snapshots to a
 * JavaScript worklet on the UI runtime so the UI can be animated from live
 * audio (e.g. amplitude/RMS visualizers).
 *
 * Incoming audio is down-mixed to mono, accumulated until `bufferLength` is
 * reached, then dispatched to the UI scheduler as a single `Float32Array`.
 * While the UI callback is running, incoming frames are skipped. Audio flows
 * through unchanged. The node is always scheduled while the context is running
 * (like `AnalyserNode`), but snapshot accumulation runs only when upstream
 * inputs are connected.
 */
class WorkletNode : public audioapi::AudioNode {
 public:
  WorkletNode(
      const std::shared_ptr<audioapi::BaseAudioContext> &context,
      UIWorkletsRunner workletRunner,
      size_t bufferLength);

  ~WorkletNode() override;

  DELETE_COPY_AND_MOVE(WorkletNode);

 protected:
  void processNode(int framesToProcess) override;

  void processInputs(const std::vector<const audioapi::DSPAudioBuffer *> &inputs, int numFrames)
      override;

 private:
  void dispatchToUI();

  std::unique_ptr<audioapi::DSPAudioBuffer> downMixBuffer_;

  UIWorkletsRunner workletRunner_;
  size_t bufferLength_;
  size_t framesFilled_{0};

  /// @brief True while a UI-thread worklet invocation is still pending. Snapshot buffers
  /// are not filled until the callback completes.
  std::shared_ptr<std::atomic<bool>> busy_;
};

} // namespace audioworklets
