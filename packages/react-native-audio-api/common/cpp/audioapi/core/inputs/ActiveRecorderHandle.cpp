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
  std::scoped_lock lock(mutex_);
  recorder_ = recorder;
}

void ActiveRecorderHandle::clearRecorder(const AudioRecorder *recorder) {
  std::shared_ptr<AudioRecorder> current;
  {
    std::scoped_lock lock(mutex_);
    current = recorder_.lock();
    if (current != nullptr && current.get() != recorder) {
      return;
    }
    recorder_.reset();
  }
}

bool ActiveRecorderHandle::isRecordingOngoing() {
  std::shared_ptr<AudioRecorder> recorder;
  {
    std::scoped_lock lock(mutex_);
    recorder = recorder_.lock();
  }
  return recorder != nullptr && !recorder->isIdle();
}

bool ActiveRecorderHandle::pauseActiveRecording() {
  std::shared_ptr<AudioRecorder> recorder;
  {
    std::scoped_lock lock(mutex_);
    recorder = recorder_.lock();
  }
  if (recorder == nullptr || !recorder->isRecording()) {
    return false;
  }
  recorder->pause();
  return true;
}

bool ActiveRecorderHandle::resumeActiveRecording() {
  std::shared_ptr<AudioRecorder> recorder;
  {
    std::scoped_lock lock(mutex_);
    recorder = recorder_.lock();
  }
  if (recorder == nullptr || !recorder->isPaused()) {
    return false;
  }
  recorder->resume();
  return true;
}

bool ActiveRecorderHandle::stopActiveRecording() {
  std::shared_ptr<AudioRecorder> recorder;
  {
    std::scoped_lock lock(mutex_);
    recorder = recorder_.lock();
  }
  if (recorder == nullptr || recorder->isIdle()) {
    return false;
  }

  // stop() blocks for as long as file finalization takes (possibly seconds), so
  // mutex_ is released around it to keep isRecordingOngoing(), setRecorder() and
  // clearRecorder() (e.g. from ~AudioRecorderHostObject) responsive meanwhile.
  auto result = recorder->stop();
  if (!result.is_ok()) {
    return false;
  }

  auto [paths, size, duration] = result.unwrap();
  if (!paths.empty()) {
    std::scoped_lock lock(mutex_);
    lastResult_ =
        RecordingStopResult{.paths = std::move(paths), .size = size, .duration = duration};
  }
  return true;
}

std::optional<RecordingStopResult> ActiveRecorderHandle::takeLastRecordingResult() {
  std::scoped_lock lock(mutex_);
  return std::exchange(lastResult_, std::nullopt);
}

} // namespace audioapi
