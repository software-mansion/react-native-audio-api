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

/// @brief The process-wide view of the recording session that is currently active.
///
/// The JavaScript host object owns the AudioRecorder. Native code still needs to reach that
/// recorder when JS is dead, so this handle keeps a non-owning reference to the recorder that
/// holds the session, forwards to it, and keeps what the last session recorded until JS asks for
/// it. Recorders report to it through IRecordingSessionRegistry; what a session does, and how it
/// ends, is the recorder's.
class ActiveRecorderHandle final : public IRecordingSessionRegistry {
 public:
  static ActiveRecorderHandle &global();
  DELETE_COPY_AND_MOVE(ActiveRecorderHandle);
  ~ActiveRecorderHandle() override = default;

  bool claim(const std::shared_ptr<AudioRecorder> &recorder) override;
  /// A recording that names no file is not kept.
  void ended(const AudioRecorder &recorder, const std::optional<FileInfo> &recording) override;

  /// @brief The state of the recorder that holds the session, or Idle when none does.
  [[nodiscard]] RecorderState currentState() const;

  /// @brief True from a recorder's claim until its session has ended: while it starts, records,
  /// is paused, and while its files are being closed. Once this turns false, what the session
  /// recorded is available to consumeLastRecordingResult().
  [[nodiscard]] bool isRecordingOngoing() const;

  /// @brief AudioRecorder::pause() on the recorder that holds the session.
  /// @return Its state afterwards; Idle when no recorder holds the session.
  RecorderState pause();

  /// @brief AudioRecorder::resume() on the recorder that holds the session. A refused resume
  /// leaves the session paused; the returned state shows it.
  RecorderState resume();

  /// @brief AudioRecorder::rerouteInput() on the recorder that holds the session; Ok when none
  /// does.
  Result<NoneType, std::string> rerouteInput();

  /// @brief AudioRecorder::stop() on the recorder that holds the session. Blocks until the
  /// output file is finalized. Because of this, don't call on a UI thread.
  Result<FileInfo, std::string> stopAndReturnInfo();

  /// @brief stopAndReturnInfo(), returning the state afterwards instead of the recording.
  RecorderState stopAndReturnState();

  /// @brief Consume-once: returns what the last ended session recorded and clears it, or
  /// std::nullopt when nothing is kept.
  std::optional<FileInfo> consumeLastRecordingResult();

 private:
  ActiveRecorderHandle() = default;
  friend struct ActiveRecorderHandleTestPeer;

  [[nodiscard]] std::shared_ptr<AudioRecorder> sessionHolder() const;

  /// Guards the two members below. Never held across a call into a recorder, which can take
  /// seconds to start and blocks on a file close to stop.
  mutable std::mutex mutex_;
  std::weak_ptr<AudioRecorder> recorder_;
  std::optional<FileInfo> lastResult_;
};

} // namespace audioapi
