#pragma once

#include <audioapi/core/inputs/FileInfo.h>
#include <audioapi/core/inputs/IRecordingSessionRegistry.h>
#include <audioapi/core/inputs/RecorderState.h>
#include <audioapi/utils/Macros.h>
#include <audioapi/utils/Result.hpp>

#include <memory>
#include <mutex>
#include <optional>
#include <string>

namespace audioapi {

class AudioRecorder;

/// @brief The process's recording session, for native code that has no recorder of its own:
/// notification actions, audio-session deactivation, input routing, and a recording that outlived
/// its UI. The app's recorders report their sessions here, so the process records at most one at a
/// time.
///
/// mutex_ guards only the slot and the kept recording, and is never held while calling a recorder:
/// a query does not wait behind a start's route wait or a stop's file close. Under it the slot is
/// only compared by owner, never locked: a shared_ptr made there could become the recorder's last
/// owner and run its destructor, which releases the slot, under the mutex.
class ActiveRecorderHandle final : public IRecordingSessionRegistry {
 public:
  /// The handle native code reaches and the app's recorders report to.
  static const std::shared_ptr<ActiveRecorderHandle> &global();

  ActiveRecorderHandle() = default;
  ~ActiveRecorderHandle() override = default;
  DELETE_COPY_AND_MOVE(ActiveRecorderHandle);

  Result<RecordingSessionId, std::string> claim(
      const std::shared_ptr<AudioRecorder> &recorder) override;
  void release(const std::weak_ptr<AudioRecorder> &recorder) override;
  /// Kept only when the recording wrote files and no later session's recording was kept before.
  void keepRecording(RecordingSessionId session, FileInfo recording) override;

  /// The occupant's state, or Idle when the slot is empty.
  [[nodiscard]] RecorderState currentState() const;

  /// A paused recording counts as ongoing: it still owns an open output file.
  [[nodiscard]] bool isRecordingOngoing() const;

  /// The occupant's AudioRecorder::pause(); returns the state it leaves.
  RecorderState pause();

  /// The occupant's AudioRecorder::resume(); returns the state it leaves, so a refused resume
  /// reads as Paused.
  RecorderState resume();

  /// The occupant's AudioRecorder::rerouteInput(); Ok when the slot is empty.
  Result<NoneType, std::string> rerouteInput();

  /// The occupant's AudioRecorder::stop(). Blocks until the output file is closed, so not for a UI
  /// thread.
  Result<FileInfo, std::string> stopAndReturnInfo();

  /// stopAndReturnInfo(), then the state it leaves.
  RecorderState stopAndReturnState();

  /// Consume-once: the recording kept by the last stop, or std::nullopt.
  std::optional<FileInfo> consumeLastRecordingResult();

 private:
  [[nodiscard]] std::shared_ptr<AudioRecorder> occupant() const;

  mutable std::mutex mutex_;
  std::weak_ptr<AudioRecorder> occupant_;
  std::optional<FileInfo> keptRecording_;
  RecordingSessionId lastClaimedSession_ = 0;
  /// Outlives consumption, so a late recording of an older session is not kept after it.
  RecordingSessionId keptSession_ = 0;
};

} // namespace audioapi
