#pragma once

#include <audioapi/compatibility/StableAPI.h>
#include <audioworklets/UIWorkletsRunner.h>
#include <audioworklets/core/UIWorkletSnapshots.h>
#include <audioworklets/core/WorkletNodeDomain.h>
#include <audioworklets/types/NodeOptions.h>

#include <cstddef>
#include <memory>
#include <vector>

namespace audioworklets {

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

  DELETE_COPY_AND_MOVE(WorkletNode);

  /// Snapshot settings (buffer length, smoothing) are read and changed here.
  [[nodiscard]] UIWorkletSnapshots &snapshots() {
    return snapshots_;
  }

 protected:
  void processNode(int framesToProcess) override;

  void processInputs(const std::vector<const audioapi::DSPAudioBuffer *> &inputs, int numFrames)
      override;

 private:
  std::unique_ptr<audioapi::DSPAudioBuffer> downMixBuffer_;
  UIWorkletSnapshots snapshots_;
};

} // namespace audioworklets
