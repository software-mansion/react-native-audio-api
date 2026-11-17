#include <audioapi/core/inputs/RecordingSessionRegistry.h>

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

Result<RecordingSessionId, std::string> RecordingSessionRegistry::claim(
    const std::shared_ptr<AudioRecorder> &recorder) {
  std::scoped_lock lock(mutex_);

  if (!occupant_.expired() && !isSameRecorder(occupant_, recorder)) {
    return Err("Another recording is already in progress");
  }

  occupant_ = recorder;
  return Result<RecordingSessionId, std::string>::Ok(++lastClaimedSession_);
}

void RecordingSessionRegistry::release(const std::weak_ptr<AudioRecorder> &recorder) {
  std::scoped_lock lock(mutex_);

  if (isSameRecorder(occupant_, recorder)) {
    occupant_.reset();
  }
}

void RecordingSessionRegistry::keepRecording(RecordingSessionId session, FileInfo recording) {
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

std::optional<FileInfo> RecordingSessionRegistry::consumeLastRecordingResult() {
  std::scoped_lock lock(mutex_);
  return std::exchange(keptRecording_, std::nullopt);
}

std::shared_ptr<AudioRecorder> RecordingSessionRegistry::occupant() const {
  std::scoped_lock lock(mutex_);
  return occupant_.lock();
}

} // namespace audioapi
