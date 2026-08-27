#pragma once

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
/// Assumes at most one AudioRecorder is alive at a time; setting a new recorder replaces
/// the previous one.
class ActiveRecorderHandle {
 public:
  static ActiveRecorderHandle &global();

  void setRecorder(const std::shared_ptr<AudioRecorder> &recorder);

  /// @brief Detaches the recorder, but only if the slot still holds @p recorder.
  void clearRecorder(const AudioRecorder *recorder);

  /// @brief True while a recording session is active; a paused recording counts as
  /// ongoing because it still owns an open output file.
  bool isRecordingOngoing();

  /// @return true if an actively recording session was paused by this call.
  bool pauseActiveRecording();

  /// @return true if a paused session was resumed by this call.
  bool resumeActiveRecording();

  /// @brief Stops a non-idle recording and stashes its file info for
  /// takeLastRecordingResult(). Blocks until the output file is finalized —
  /// never call on a UI thread.
  /// @return true if this call stopped the recording. Losing a race with a
  /// JS-initiated stop() returns false; the JS promise delivers that result.
  bool stopActiveRecording();

  /// @brief Consume-once: returns the file info stashed by stopActiveRecording()
  /// and clears it, or std::nullopt when nothing is stashed.
  std::optional<RecordingStopResult> takeLastRecordingResult();

 private:
  std::mutex mutex_;
  std::weak_ptr<AudioRecorder> recorder_;
  std::optional<RecordingStopResult> lastResult_;
};

} // namespace audioapi
