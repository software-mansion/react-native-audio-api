#include <audioapi/core/inputs/ActiveRecorderHandle.h>

#include <audioapi/core/inputs/AudioRecorder.h>

#include <memory>
#include <tuple>
#include <utility>

namespace audioapi {

ActiveRecorderHandle &ActiveRecorderHandle::global() {
  static ActiveRecorderHandle handle;
  return handle;
}

void ActiveRecorderHandle::setRecorder(const std::shared_ptr<AudioRecorder> &recorder) {
  std::scoped_lock lock(destructorMutex_);
  recorder_ = recorder;
}

void ActiveRecorderHandle::clearRecorder(const AudioRecorder *recorder) {
  std::shared_ptr<AudioRecorder> current;
  {
    std::scoped_lock lock(destructorMutex_);
    current = recorder_.lock();
    if (current && current.get() != recorder) {
      return;
    }
    recorder_.reset();
  }
}

RecorderState ActiveRecorderHandle::currentState() {
  std::scoped_lock lock(destructorMutex_);
  std::shared_ptr<AudioRecorder> recorder = recorder_.lock();
  return stateOf(recorder);
}

bool ActiveRecorderHandle::isRecordingOngoing() {
  return currentState() != RecorderState::Idle;
}

RecorderState ActiveRecorderHandle::pauseActiveRecording() {
  std::scoped_lock lock(destructorMutex_);
  std::shared_ptr<AudioRecorder> recorder = recorder_.lock();
  if (recorder && recorder->isRecording()) {
    recorder->pause();
  }
  return stateOf(recorder);
}

RecorderState ActiveRecorderHandle::resumeActiveRecording() {
  std::scoped_lock lock(destructorMutex_);
  std::shared_ptr<AudioRecorder> recorder = recorder_.lock();
  if (recorder && recorder->isPaused()) {
    recorder->resume();
  }
  return stateOf(recorder);
}

RecorderState ActiveRecorderHandle::stopActiveRecording() {
  std::scoped_lock lock(destructorMutex_);
  std::shared_ptr<AudioRecorder> recorder = recorder_.lock();
  if (!recorder || recorder->isIdle()) {
    return stateOf(recorder);
  }

  auto result = recorder->stop();
  if (!result.is_ok()) {
    return stateOf(recorder);
  }

  auto [paths, size, duration] = result.unwrap();
  if (!paths.empty()) {
    lastResult_ =
        RecordingStopResult{.paths = std::move(paths), .size = size, .duration = duration};
  }
  return stateOf(recorder);
}

std::optional<RecordingStopResult> ActiveRecorderHandle::takeLastRecordingResult() {
  std::scoped_lock lock(destructorMutex_);
  return std::exchange(lastResult_, std::nullopt);
}

RecorderState ActiveRecorderHandle::stateOf(const std::shared_ptr<AudioRecorder> &recorder) {
  return recorder ? recorder->getState() : RecorderState::Idle;
}

} // namespace audioapi
