#include <audioapi/core/inputs/ActiveRecorderHandle.h>

#include <audioapi/core/inputs/AudioRecorder.h>

#include <memory>
#include <string>
#include <utility>

namespace audioapi {

ActiveRecorderHandle &ActiveRecorderHandle::global() {
  static ActiveRecorderHandle handle;
  return handle;
}

bool ActiveRecorderHandle::claim(const std::shared_ptr<AudioRecorder> &recorder) {
  std::scoped_lock lock(mutex_);

  if (!recorder_.expired()) {
    return false;
  }

  recorder_ = recorder;
  return true;
}

void ActiveRecorderHandle::ended(
    const AudioRecorder &recorder,
    const std::optional<FileInfo> &recording) {
  std::scoped_lock lock(mutex_);

  // A recorder that ends its session from its destructor has already expired here, and another
  // recorder may have claimed the session since.
  const auto holder = recorder_.lock();
  if (holder == nullptr || holder.get() == &recorder) {
    recorder_.reset();
  }

  if (recording.has_value() && !recording->paths.empty()) {
    lastResult_ = recording;
  }
}

std::shared_ptr<AudioRecorder> ActiveRecorderHandle::sessionHolder() const {
  std::scoped_lock lock(mutex_);
  return recorder_.lock();
}

RecorderState ActiveRecorderHandle::currentState() const {
  const auto recorder = sessionHolder();
  return recorder != nullptr ? recorder->getState() : RecorderState::Idle;
}

bool ActiveRecorderHandle::isRecordingOngoing() const {
  std::scoped_lock lock(mutex_);
  return !recorder_.expired();
}

RecorderState ActiveRecorderHandle::pause() {
  const auto recorder = sessionHolder();
  if (recorder == nullptr) {
    return RecorderState::Idle;
  }

  recorder->pause();
  return recorder->getState();
}

RecorderState ActiveRecorderHandle::resume() {
  const auto recorder = sessionHolder();
  if (recorder == nullptr) {
    return RecorderState::Idle;
  }

  recorder->resume();
  return recorder->getState();
}

Result<NoneType, std::string> ActiveRecorderHandle::rerouteInput() {
  const auto recorder = sessionHolder();
  if (recorder == nullptr) {
    return Result<NoneType, std::string>::Ok(None);
  }

  return recorder->rerouteInput();
}

Result<FileInfo, std::string> ActiveRecorderHandle::stopAndReturnInfo() {
  const auto recorder = sessionHolder();
  if (recorder == nullptr) {
    return Result<FileInfo, std::string>::Err("Recorder is not in recording state.");
  }

  return recorder->stop();
}

RecorderState ActiveRecorderHandle::stopAndReturnState() {
  stopAndReturnInfo();
  return currentState();
}

std::optional<FileInfo> ActiveRecorderHandle::consumeLastRecordingResult() {
  std::scoped_lock lock(mutex_);
  return std::exchange(lastResult_, std::nullopt);
}

} // namespace audioapi
