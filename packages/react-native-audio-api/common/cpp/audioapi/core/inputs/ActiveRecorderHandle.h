#pragma once

#include <audioapi/core/inputs/RecorderState.h>
#include <audioapi/utils/Macros.h>
#include <audioapi/utils/Result.hpp>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace audioapi {

class AudioRecorder;

struct FileInfo {
  std::vector<std::string> paths;
  double size;
  double duration;
};

/// @brief The process-wide view of the recording session that is currently active.
///
/// The JavaScript host object owns the AudioRecorder. Native code still needs
/// to reach that recorder when JavaScript is not in the picture.
/// This handle publishes a non-owning reference to the recorder
/// so those actions can reach it. It does not extend the recorder's lifetime:
/// if nothing else still owns it, there is no session to control. Only one
/// recording session is published at a time.
class ActiveRecorderHandle {
 public:
  static ActiveRecorderHandle &global();
  DELETE_COPY_AND_MOVE(ActiveRecorderHandle);
  ~ActiveRecorderHandle() = default;

  /// @brief Starts @p recorder and stores it in the slot unless another non-idle
  /// recorder already occupies it. The slot is assigned only if start succeeds.
  Result<NoneType, std::string> tryStart(
      const std::shared_ptr<AudioRecorder> &recorder,
      const std::string &fileNameOverride);

  /// @brief Drops the slot without stopping the recorder.
  void clearRecorder();

  /// @brief Drops the slot only if it still holds @p expected.
  /// A foreign occupant is left in place.
  void clearRecorder(const std::shared_ptr<AudioRecorder> &expected);

  /// @brief The state of the recorder in the slot, or Idle when the slot is empty.
  [[nodiscard]] RecorderState currentState() const;

  /// @brief True while a recording session is active; a paused recording counts as
  /// ongoing because it still owns an open output file.
  [[nodiscard]] bool isRecordingOngoing() const;

  /// @brief Pauses an actively recording session; a no-op in any other state.
  RecorderState pause();

  /// @brief Resumes a paused session; a no-op in any other state.
  RecorderState resume();

  /// @brief Stops the occupant and returns AudioRecorder::stop()'s Result,
  /// including the original error. On success with non-empty paths, stashes a
  /// copy for consumeLastRecordingResult() and clears the slot. Blocks until
  /// the output file is finalized. Because of this, don't call on a UI thread.
  Result<FileInfo, std::string> stopAndReturnInfo();

  /// @brief stopAndReturnInfo() if the slot still holds @p expected; otherwise
  /// returns an error and does not stop the occupant.
  Result<FileInfo, std::string> stopAndReturnInfo(const std::shared_ptr<AudioRecorder> &expected);

  /// @brief Stops the occupant (same as stopAndReturnInfo) and returns the
  /// resulting slot state. The stop Result is discarded; file info is still stashed.
  RecorderState stopAndReturnState();

  /// @brief Consume-once: returns the file info stashed by stopAndReturnInfo()
  /// and clears it, or std::nullopt when nothing is stashed.
  std::optional<FileInfo> consumeLastRecordingResult();

 private:
  ActiveRecorderHandle() = default;
  friend struct ActiveRecorderHandleTestPeer;

  static RecorderState stateOf(const std::shared_ptr<AudioRecorder> &recorder);

  mutable std::recursive_mutex mutex_;
  std::weak_ptr<AudioRecorder> recorder_;
  std::optional<FileInfo> lastResult_;
};

} // namespace audioapi
