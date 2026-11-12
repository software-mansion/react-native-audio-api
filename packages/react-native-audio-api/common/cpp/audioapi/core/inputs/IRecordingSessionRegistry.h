#pragma once

#include <audioapi/core/inputs/FileInfo.h>

#include <memory>
#include <optional>

namespace audioapi {

class AudioRecorder;

/// @brief What a recorder tells the process about its recording session. The process allows one
/// session at a time, and keeps what a session recorded until someone asks for it.
class IRecordingSessionRegistry {
 public:
  virtual ~IRecordingSessionRegistry() = default;

  /// Reserves the session for @p recorder, from before it starts until ended(). False while
  /// another recorder holds it.
  virtual bool claim(const std::shared_ptr<AudioRecorder> &recorder) = 0;

  /// Gives the session back together with what it recorded; nullopt when it never started, or
  /// when its outputs failed to close.
  virtual void ended(const AudioRecorder &recorder, const std::optional<FileInfo> &recording) = 0;
};

} // namespace audioapi
