#include <audioapi/core/inputs/RecordingSessionManager.h>

#include <memory>
#include <optional>
#include <string>
#include <utility>

namespace audioapi {

namespace {

/// Whether @p a and @p b share a control block: true for an expired pointer too, and creates no
/// owner.
bool isSameRecorder(const std::weak_ptr<AudioRecorder> &a, const std::weak_ptr<AudioRecorder> &b) {
  return !a.owner_before(b) && !b.owner_before(a);
}

} // namespace

const std::shared_ptr<RecordingSessionManager> &RecordingSessionManager::global() {
  static const auto manager = std::make_shared<RecordingSessionManager>();
  return manager;
}

Result<uint64_t, std::string> RecordingSessionManager::claim(
    const std::shared_ptr<AudioRecorder> &recorder) {
  std::scoped_lock lock(mutex_);

  if (occupant_ && !isSameRecorder(*occupant_, recorder)) {
    return Err("Another recording is already in progress");
  }

  occupant_ = recorder;
  return Result<uint64_t, std::string>::Ok(++lastClaimedSession_);
}

void RecordingSessionManager::release(const std::weak_ptr<AudioRecorder> &recorder) {
  std::scoped_lock lock(mutex_);

  if (occupant_ && isSameRecorder(*occupant_, recorder)) {
    occupant_.reset();
  }
}

void RecordingSessionManager::saveRecording(uint64_t session, FileInfo recording) {
  if (recording.paths.empty()) {
    return;
  }

  std::scoped_lock lock(mutex_);
  if (session < keptSession_) {
    return;
  }

  keptRecording_ = std::move(recording);
  keptSession_ = session;
}

std::optional<FileInfo> RecordingSessionManager::consumeLastRecordingResult() {
  std::scoped_lock lock(mutex_);
  return std::exchange(keptRecording_, std::nullopt);
}

bool RecordingSessionManager::isRecordingOngoing() const {
  std::scoped_lock lock(mutex_);
  // An expired recorder still holds the claim while its destructor closes its consumers; that is
  // not ongoing.
  return occupant_.has_value() && !occupant_->expired();
}

std::shared_ptr<AudioRecorder> RecordingSessionManager::occupant() const {
  std::scoped_lock lock(mutex_);
  return occupant_ ? occupant_->lock() : nullptr;
}

} // namespace audioapi
