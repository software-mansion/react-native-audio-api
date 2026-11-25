#include <audioapi/core/inputs/ActiveRecorderHandle.h>

#include <audioapi/core/inputs/AudioRecorder.h>

#include <memory>
#include <string>

namespace audioapi {

ActiveRecorderHandle &ActiveRecorderHandle::global() {
  static ActiveRecorderHandle handle(RecordingSessionManager::global());
  return handle;
}

RecorderState ActiveRecorderHandle::currentState() const {
  auto recorder = sessionManager_->occupant();
  return recorder != nullptr ? recorder->getState() : RecorderState::Idle;
}

RecorderState ActiveRecorderHandle::pause() {
  auto recorder = sessionManager_->occupant();
  if (recorder == nullptr) {
    return RecorderState::Idle;
  }

  recorder->pause();
  return recorder->getState();
}

RecorderState ActiveRecorderHandle::resume() {
  auto recorder = sessionManager_->occupant();
  if (recorder == nullptr) {
    return RecorderState::Idle;
  }

  recorder->resume();
  return recorder->getState();
}

Result<NoneType, std::string> ActiveRecorderHandle::rerouteInput() {
  auto recorder = sessionManager_->occupant();
  if (recorder == nullptr) {
    return Ok(None);
  }

  return recorder->rerouteInput();
}

Result<FileInfo, std::string> ActiveRecorderHandle::stopAndReturnInfo() {
  auto recorder = sessionManager_->occupant();
  if (recorder == nullptr) {
    return Err("Recorder is not in recording state.");
  }

  return recorder->stop();
}

RecorderState ActiveRecorderHandle::stopAndReturnState() {
  stopAndReturnInfo();
  return currentState();
}

} // namespace audioapi
