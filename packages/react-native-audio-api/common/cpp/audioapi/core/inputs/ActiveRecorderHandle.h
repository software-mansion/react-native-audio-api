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
/// The recorder is otherwise owned solely by its JS-side host object, so platform code
/// (e.g. the Android recording-notification STOP action) has no way to reach it once the
/// JS runtime is unreachable, and a fresh JS context has no way to learn that a recording
/// outlived the app UI. This handle closes both gaps: it can stop the recording natively
/// and it stashes the resulting file info until JS collects it.
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
