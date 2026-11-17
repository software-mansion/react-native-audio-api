#include <audioapi/core/inputs/ActiveRecorderHandle.h>

#include <audioapi/core/inputs/AudioRecorder.h>

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

const std::shared_ptr<ActiveRecorderHandle> &ActiveRecorderHandle::global() {
  static const auto handle = std::make_shared<ActiveRecorderHandle>();
  return handle;
}

Result<RecordingSessionId, std::string> ActiveRecorderHandle::claim(
    const std::shared_ptr<AudioRecorder> &recorder) {
  std::scoped_lock lock(mutex_);

  if (!occupant_.expired() && !isSameRecorder(occupant_, recorder)) {
    return Err("Another recording is already in progress");
  }

  occupant_ = recorder;
  return Result<RecordingSessionId, std::string>::Ok(++lastClaimedSession_);
}

void ActiveRecorderHandle::release(const std::weak_ptr<AudioRecorder> &recorder) {
  std::scoped_lock lock(mutex_);

  if (isSameRecorder(occupant_, recorder)) {
    occupant_.reset();
  }
}

void ActiveRecorderHandle::keepRecording(RecordingSessionId session, FileInfo recording) {
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

RecorderState ActiveRecorderHandle::currentState() const {
  auto recorder = occupant();
  return recorder != nullptr ? recorder->getState() : RecorderState::Idle;
}

bool ActiveRecorderHandle::isRecordingOngoing() const {
  return currentState() != RecorderState::Idle;
}

RecorderState ActiveRecorderHandle::pause() {
  auto recorder = occupant();
  if (recorder == nullptr) {
    return RecorderState::Idle;
  }

  recorder->pause();
  return recorder->getState();
}

RecorderState ActiveRecorderHandle::resume() {
  auto recorder = occupant();
  if (recorder == nullptr) {
    return RecorderState::Idle;
  }

  recorder->resume();
  return recorder->getState();
}

Result<NoneType, std::string> ActiveRecorderHandle::rerouteInput() {
  auto recorder = occupant();
  if (recorder == nullptr) {
    return Ok(None);
  }

  return recorder->rerouteInput();
}

Result<FileInfo, std::string> ActiveRecorderHandle::stopAndReturnInfo() {
  auto recorder = occupant();
  if (recorder == nullptr) {
    return Err("Recorder is not in recording state.");
  }

  return recorder->stop();
}

RecorderState ActiveRecorderHandle::stopAndReturnState() {
  stopAndReturnInfo();
  return currentState();
}

std::optional<FileInfo> ActiveRecorderHandle::consumeLastRecordingResult() {
  std::scoped_lock lock(mutex_);
  return std::exchange(keptRecording_, std::nullopt);
}

std::shared_ptr<AudioRecorder> ActiveRecorderHandle::occupant() const {
  std::scoped_lock lock(mutex_);
  return occupant_.lock();
}

} // namespace audioapi
