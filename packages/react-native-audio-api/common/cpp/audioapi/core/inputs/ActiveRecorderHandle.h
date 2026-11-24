#pragma once

#include <audioapi/core/inputs/FileInfo.h>
#include <audioapi/core/inputs/RecorderState.h>
#include <audioapi/core/inputs/RecordingSessionManager.h>
#include <audioapi/utils/Result.hpp>

#include <memory>
#include <string>
#include <utility>

namespace audioapi {

/// @brief Acts on whichever recorder owns the recording session, for native code that has no
/// recorder of its own: notification actions, audio-session deactivation, input routing, and a
/// recording that outlived its UI. Its forwards call that recorder with no lock held.
class ActiveRecorderHandle final {
 public:
  explicit ActiveRecorderHandle(std::shared_ptr<RecordingSessionManager> sessionManager)
      : sessionManager_(std::move(sessionManager)) {}

  /// The handle native code reaches, over RecordingSessionManager::global().
  static ActiveRecorderHandle &global();

  /// The occupant's state, or Idle when the slot is empty.
  [[nodiscard]] RecorderState currentState() const;

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

 private:
  std::shared_ptr<RecordingSessionManager> sessionManager_;
};

} // namespace audioapi
