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

/// @brief Where recorders report their sessions: admits one at a time, and keeps the newest
/// recording for consumeLastRecordingResult().
///
/// Recorders call claim() and release() holding their control mutex, so mutex_ is a leaf: nothing
/// under it calls a recorder. Under it the slot is only compared by owner, never locked: a
/// shared_ptr made there could become the recorder's last owner and run its destructor, which
/// releases the slot, under the mutex.
class RecordingSessionRegistry {
 public:
  RecordingSessionRegistry() = default;
  virtual ~RecordingSessionRegistry() = default;
  DELETE_COPY_AND_MOVE(RecordingSessionRegistry);

  /// A session of @p recorder is starting; returns its id. Fails while another recorder's session
  /// holds the claim.
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

 protected:
  /// The live session's recorder, or nullptr. Locked outside the mutex, so the copy is safe to be
  /// its last owner.
  [[nodiscard]] std::shared_ptr<AudioRecorder> occupant() const;

 private:
  mutable std::mutex mutex_;
  std::weak_ptr<AudioRecorder> occupant_;
  std::optional<FileInfo> keptRecording_;
  uint64_t lastClaimedSession_ = 0;
  /// Outlives consumption, so a late recording of an older session is not kept after it.
  uint64_t keptSession_ = 0;
};

} // namespace audioapi
