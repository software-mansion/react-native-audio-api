#pragma once

#include <audioapi/core/inputs/IRecordingSessionRegistry.h>

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

// NOLINTBEGIN

namespace audioapi::test_support {

/// Records what a recorder reports about its sessions. It admits every claim unless the test
/// scripts a refusal; the one-session rule itself is ActiveRecorderHandle's, tested there.
class FakeSessionRegistry final : public IRecordingSessionRegistry {
 public:
  /// When set, claim() refuses with this message.
  std::optional<std::string> claimError;

  int claims = 0;
  int releases = 0;
  /// Everything keepRecording() received, including recordings without files, and the session
  /// each came from.
  std::vector<FileInfo> keptRecordings;
  std::vector<RecordingSessionId> keptSessions;

  /// Numbers sessions by claim: the first claim is session 1.
  Result<RecordingSessionId, std::string> claim(const std::shared_ptr<AudioRecorder> &) override {
    ++claims;
    if (claimError.has_value()) {
      return Result<RecordingSessionId, std::string>::Err(*claimError);
    }
    return Result<RecordingSessionId, std::string>::Ok(static_cast<RecordingSessionId>(claims));
  }

  void release(const std::weak_ptr<AudioRecorder> &) override {
    ++releases;
  }

  void keepRecording(RecordingSessionId session, FileInfo recording) override {
    keptSessions.push_back(session);
    keptRecordings.push_back(std::move(recording));
  }
};

} // namespace audioapi::test_support

// NOLINTEND
