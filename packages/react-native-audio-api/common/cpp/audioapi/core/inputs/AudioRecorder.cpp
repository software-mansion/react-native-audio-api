
#include <audioapi/core/inputs/AudioRecorder.h>
#include <audioapi/core/utils/AudioFileWriter.h>
#include <audioapi/core/utils/AudioRecorderCallback.h>

namespace audioapi {

/// @brief Sets the error callback to be invoked when an error occurs during recording.
/// This method should be called from the JS thread only.
/// @param callbackId Identifier for the JS callback to be invoked.
void AudioRecorder::setOnErrorCallback(uint64_t callbackId) {
  std::scoped_lock lock(callbackMutex_, fileWriterMutex_, errorCallbackMutex_);

  if (usesFileOutput()) {
    fileWriter_->setOnErrorCallback(callbackId);
  }

  if (usesCallback()) {
    dataCallback_->setOnErrorCallback(callbackId);
  }

  errorCallbackId_.store(callbackId, std::memory_order_release);
}

/// @brief Clears the error callback.
/// If the recorder is currently active, it will stop invoking the callback immediately.
/// This method should be called from the JS thread only.
void AudioRecorder::clearOnErrorCallback() {
  std::scoped_lock lock(callbackMutex_, fileWriterMutex_, errorCallbackMutex_);

  if (usesFileOutput()) {
    fileWriter_->clearOnErrorCallback();
  }

  if (usesCallback()) {
    dataCallback_->clearOnErrorCallback();
  }

  errorCallbackId_.store(0, std::memory_order_release);
}

/// @brief Gets the current duration of the recorded audio in seconds.
/// @returns Duration in seconds.
double AudioRecorder::getCurrentDuration() const {
  double duration = 0.0;

  if (usesFileOutput()) {
    duration = fileWriter_->getCurrentDuration();
  }

  return duration;
}

bool AudioRecorder::usesCallback() const {
  return callbackOutputEnabled_.load(std::memory_order_acquire);
}

bool AudioRecorder::usesFileOutput() const {
  return fileOutputEnabled_.load(std::memory_order_acquire);
}

bool AudioRecorder::isConnected() const {
  return isConnected_.load(std::memory_order_acquire);
}

} // namespace audioapi
