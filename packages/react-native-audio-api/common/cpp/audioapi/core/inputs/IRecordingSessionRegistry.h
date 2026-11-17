#pragma once

#include <audioapi/core/inputs/FileInfo.h>
#include <audioapi/utils/Macros.h>
#include <audioapi/utils/Result.hpp>

#include <cstdint>
#include <memory>
#include <string>

namespace audioapi {

class AudioRecorder;

/// Numbers sessions in the order they were claimed.
using RecordingSessionId = uint64_t;

/// @brief Where a recorder reports its recording sessions, so the process records at most one at
/// a time and native code can reach it (ActiveRecorderHandle).
class IRecordingSessionRegistry {
 public:
  IRecordingSessionRegistry() = default;
  virtual ~IRecordingSessionRegistry() = default;
  DELETE_COPY_AND_MOVE(IRecordingSessionRegistry);

  /// A session of @p recorder is starting; returns its id. Fails while another recorder's session
  /// holds the claim.
  virtual Result<RecordingSessionId, std::string> claim(
      const std::shared_ptr<AudioRecorder> &recorder) = 0;

  /// The session of @p recorder has ended, or its start failed after claim(). Also called from the
  /// recorder's destructor, where @p recorder has expired but still identifies it.
  virtual void release(const std::weak_ptr<AudioRecorder> &recorder) = 0;

  /// What @p session wrote, once its file is closed. The claim is released before the file
  /// closes, so a later session can start and keep its recording first.
  virtual void keepRecording(RecordingSessionId session, FileInfo recording) = 0;
};

} // namespace audioapi
