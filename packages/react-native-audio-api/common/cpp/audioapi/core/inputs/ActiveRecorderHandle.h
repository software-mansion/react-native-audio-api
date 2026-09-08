#pragma once

#include <audioapi/core/inputs/RecorderState.h>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace audioapi {

class AudioRecorder;

struct RecordingStopResult {
  std::vector<std::string> paths;
  double size;
  double duration;
};

/// @brief Process-global handle to the live AudioRecorder, reachable without a JS runtime.
///
/// The recorder is owned solely by its JS-side host object, but Android's
/// recording-notification actions arrive through static JNI with no React context to
/// walk back to that object — a weak one-slot handle is the minimal bridge that lets
/// them control the live recorder. On top of native notification control it stashes,
/// consume-once, the file info of a recording finalized natively while no JS promise
/// or listener was waiting, and lets a remounted UI seed its state from the native
/// source of truth via isRecordingOngoing().
///
/// Every control method answers with the recorder state it leaves behind, so a caller
/// that has no other view of the recorder — the notification after task removal — can
/// render itself as a pure function of that state instead of tracking its own.
///
/// Assumes at most one AudioRecorder is alive at a time; setting a new recorder replaces
/// the previous one.
class ActiveRecorderHandle {
 public:
  static ActiveRecorderHandle &global();

  void setRecorder(const std::shared_ptr<AudioRecorder> &recorder);

  /// @brief Detaches the recorder, but only if the slot still holds @p recorder.
  void clearRecorder(const AudioRecorder *recorder);

  /// @brief The state of the recorder in the slot, or Idle when the slot is empty.
  RecorderState currentState();

  /// @brief True while a recording session is active; a paused recording counts as
  /// ongoing because it still owns an open output file.
  bool isRecordingOngoing();

  /// @brief Pauses an actively recording session; a no-op in any other state.
  RecorderState pauseActiveRecording();

  /// @brief Resumes a paused session; a no-op in any other state.
  RecorderState resumeActiveRecording();

  /// @brief Stops a non-idle recording and stashes its file info for
  /// takeLastRecordingResult(). Blocks until the output file is finalized —
  /// never call on a UI thread. Losing a race with a JS-initiated stop() stashes
  /// nothing; the JS promise delivers that result.
  RecorderState stopActiveRecording();

  /// @brief Consume-once: returns the file info stashed by stopActiveRecording()
  /// and clears it, or std::nullopt when nothing is stashed.
  std::optional<RecordingStopResult> takeLastRecordingResult();

 private:
  ActiveRecorderHandle() = default;
  friend struct ActiveRecorderHandleTestPeer;

  static RecorderState stateOf(const std::shared_ptr<AudioRecorder> &recorder);

  std::mutex destructorMutex_;
  std::weak_ptr<AudioRecorder> recorder_;
  std::optional<RecordingStopResult> lastResult_;
};

} // namespace audioapi
