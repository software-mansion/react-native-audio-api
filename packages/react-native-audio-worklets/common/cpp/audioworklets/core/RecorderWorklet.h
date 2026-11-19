#pragma once

#include <audioapi/compatibility/StableAPI.h>
#include <audioworklets/UIWorkletsRunner.h>
#include <audioworklets/core/UIWorkletSnapshots.h>
#include <audioworklets/core/WorkletNodeDomain.h>
#include <audioworklets/types/NodeOptions.h>

#include <cstddef>
#include <memory>
#include <string>

namespace audioworklets {

/**
 * Hands snapshots of what an `AudioRecorder` captures to a UI worklet, with no audio context:
 * the recorder's input mixed down to mono, at the input's sample rate, as PCM samples or as a
 * magnitude spectrum (see `UIWorkletSnapshots`).
 */
class RecorderWorklet final : public audioapi::AudioConsumer {
 public:
  RecorderWorklet(
      UIWorkletsRunner runner,
      WorkletNodeDomain domain,
      const WorkletNodeOptions &options);
  ~RecorderWorklet() = default;
  DELETE_COPY_AND_MOVE(RecorderWorklet);

  audioapi::Result<audioapi::NoneType, std::string> prepare(
      const audioapi::StreamFormat &format) override;
  /// Mixes the frames down to mono and adds them to the next snapshot.
  audioapi::Result<audioapi::NoneType, std::string> consume(
      const audioapi::AudioBuffer &frames,
      size_t numFrames) override;
  void close() override;
  /// Stops scheduling UI work, so no further snapshot reaches the UI runtime.
  void invalidate() override;

  /// Snapshot settings (buffer length, smoothing) are read and changed here.
  [[nodiscard]] UIWorkletSnapshots &snapshots() {
    return snapshots_;
  }

 private:
  UIWorkletSnapshots snapshots_;
  /// One channel of the input's buffer size: the input mixed down.
  std::unique_ptr<audioapi::AudioBuffer> mono_;
};

} // namespace audioworklets
