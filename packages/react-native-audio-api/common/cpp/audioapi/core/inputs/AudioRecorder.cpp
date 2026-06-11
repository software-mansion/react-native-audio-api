#include <audioapi/core/inputs/AudioRecorder.h>
#include <audioapi/core/utils/AudioFileWriter.h>
#include <audioapi/core/utils/AudioRecorderCallback.h>

namespace audioapi {

void AudioRecorder::assignOnErrorCallbackId(uint64_t callbackId) {
  std::scoped_lock lock(callbackMutex_, fileWriterMutex_, errorCallbackMutex_);

  if (usesFileOutput() && fileWriter_ != nullptr) {
    fileWriter_->assignOnErrorCallbackId(callbackId);
  }

  if (usesCallback() && dataCallback_ != nullptr) {
    dataCallback_->assignOnErrorCallbackId(callbackId);
  }

  errorEvent_.assignCallbackId(callbackId);
}

/// @brief Sets the error callback to be invoked when an error occurs during recording.
/// This method should be called from the JS thread only.
/// @param callbackId Identifier for the JS callback to be invoked.
void AudioRecorder::setOnErrorCallback(uint64_t callbackId) {
  assignOnErrorCallbackId(callbackId);
}

/// @brief Clears the error callback.
/// If the recorder is currently active, it will stop invoking the callback immediately.
/// This method should be called from the JS thread only.
void AudioRecorder::clearOnErrorCallback() {
  assignOnErrorCallbackId(0);
}

/// @brief Gets the current duration of the recorded audio in seconds.
/// @returns Duration in seconds.
double AudioRecorder::getCurrentDuration() const {
  std::scoped_lock lock(fileWriterMutex_);
  double duration = 0.0;

  if (usesFileOutput() && fileWriter_ != nullptr) {
    duration = fileWriter_->getCurrentDuration();
  }

  return duration;
}

bool AudioRecorder::usesCallback() const {
  return wantsCallback() && callbackOutputConfigured_.load(std::memory_order_acquire);
}

bool AudioRecorder::usesFileOutput() const {
  return wantsFileOutput() && fileOutputConfigured_.load(std::memory_order_acquire);
}

bool AudioRecorder::isConnected() const {
  return wantsConnection() && connectedConfigured_.load(std::memory_order_acquire);
}

bool AudioRecorder::wantsCallback() const {
  return callbackOutputEnabled_.load(std::memory_order_acquire);
}

bool AudioRecorder::wantsFileOutput() const {
  return fileOutputEnabled_.load(std::memory_order_acquire);
}

bool AudioRecorder::wantsConnection() const {
  return isConnected_.load(std::memory_order_acquire);
}

} // namespace audioapi
