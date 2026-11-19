#pragma once

#include <audioapi/core/inputs/FileInfo.h>
#include <audioapi/utils/Macros.h>
#include <audioapi/utils/Result.hpp>

#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <string>

namespace audioapi {

class AudioRecorder;

/// @brief Owns the process's recording session: admits one recorder's session at a time, and keeps
/// the newest recording for consumeLastRecordingResult(). Recorders report to it; native code that
/// has no recorder of its own reaches the session's recorder through ActiveRecorderHandle.
///
/// Recorders call claim() and release() holding their control mutex, so mutex_ is a leaf: nothing
/// under it calls a recorder. No shared_ptr to a recorder is dropped under it: as the last owner it
/// would run the recorder's destructor, which releases the slot, under the mutex. So the slot is
/// compared by owner there, and occupant() hands its copy out of the lock.
class RecordingSessionManager final {
 public:
  RecordingSessionManager() = default;
  ~RecordingSessionManager() = default;
  DELETE_COPY_AND_MOVE(RecordingSessionManager);

  /// The manager the app's recorders report to.
  static const std::shared_ptr<RecordingSessionManager> &global();

  /// A session of @p recorder is starting; returns its id. Fails while another recorder's session
  /// holds the claim, until that recorder calls release(), even once its last owner is gone.
  Result<uint64_t, std::string> claim(const std::shared_ptr<AudioRecorder> &recorder);

  /// The session of @p recorder has ended, or its start failed after claim(). Also called from the
  /// recorder's destructor, where @p recorder has expired but still identifies it.
  void release(const std::weak_ptr<AudioRecorder> &recorder);

  /// What @p session wrote, once its file is closed. Kept only when it wrote files and no later
  /// session's recording was kept before: the claim is released before the file closes, so a later
  /// session can start and keep its recording first.
  void saveRecording(uint64_t session, FileInfo recording);

  /// Consume-once: the newest kept recording, or std::nullopt.
  std::optional<FileInfo> consumeLastRecordingResult();

  /// Whether a session holds the claim, from start() until it ends. A paused recording counts as
  /// ongoing: it still owns an open output file.
  [[nodiscard]] bool isRecordingOngoing() const;

 private:
  friend class ActiveRecorderHandle;

  /// The live session's recorder, or nullptr. Locked under the mutex, which guards occupant_, and
  /// returned out of it, so the copy is dropped only after the mutex is released: safe even as the
  /// recorder's last owner.
  [[nodiscard]] std::shared_ptr<AudioRecorder> occupant() const;

  mutable std::mutex mutex_;
  /// Set from claim() until release(). The recorder inside may already have expired: it still
  /// closes its consumers in its destructor and releases only then.
  std::optional<std::weak_ptr<AudioRecorder>> occupant_;
  std::optional<FileInfo> keptRecording_;
  uint64_t lastClaimedSession_ = 0;
  /// Outlives consumption, so a late recording of an older session is not kept after it.
  uint64_t keptSession_ = 0;
};

} // namespace audioapi
