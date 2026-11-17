#pragma once

#include <audioapi/core/inputs/FileInfo.h>
#include <audioapi/core/inputs/RecorderState.h>
#include <audioapi/core/inputs/RecordingSessionRegistry.h>
#include <audioapi/utils/Result.hpp>

#include <memory>
#include <string>

namespace audioapi {

/// @brief The registry the app's recorders report to, for native code that has no recorder of its
/// own: notification actions, audio-session deactivation, input routing, and a recording that
/// outlived its UI. Its forwards call the live session's recorder with no lock held.
class ActiveRecorderHandle final : public RecordingSessionRegistry {
 public:
  /// The handle native code reaches and the app's recorders report to.
  static const std::shared_ptr<ActiveRecorderHandle> &global();

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
};

} // namespace audioapi
