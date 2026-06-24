#include <audioapi/core/inputs/AudioRecorder.h>
#include <audioapi/core/sources/RecorderAdapterNode.h>
#include <audioapi/core/utils/AudioFileWriter.h>
#include <audioapi/core/utils/AudioRecorderCallback.h>

#include <memory>
#include <string>
#include <utility>
#include <vector>

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

bool AudioRecorder::isIdle() const {
  return state_.load(std::memory_order_acquire) == RecorderState::Idle;
}

/// @brief Disables file output for the recorder. If active, the file is closed
/// immediately. The writer is detached under the lock and closed outside it.
/// This method should be called from the JS thread only.
void AudioRecorder::disableFileOutput() {
  std::shared_ptr<AudioFileWriter> fileWriter;
  {
    std::scoped_lock lock(fileWriterMutex_);
    fileOutputConfigured_.store(false, std::memory_order_release);
    fileOutputEnabled_.store(false, std::memory_order_release);
    fileWriter = std::move(fileWriter_);
  }

  if (fileWriter != nullptr) {
    fileWriter->closeFile();
  }
}

/// @brief Disconnects the currently connected RecorderAdapterNode from the recorder.
/// The adapter is detached under the lock and cleaned up outside it.
/// This method should be called from the JS thread only.
void AudioRecorder::disconnect() {
  std::shared_ptr<RecorderAdapterNode> adapterNode;
  bool hadConnection = false;
  {
    std::scoped_lock lock(adapterNodeMutex_);
    hadConnection = isConnected();
    connectedConfigured_.store(false, std::memory_order_release);
    isConnected_.store(false, std::memory_order_release);
    clearAdapterScratchState();
    adapterNode = std::move(adapterNode_);
  }

  if (hadConnection && adapterNode != nullptr) {
    adapterNode->adapterCleanup();
  }
}

/// @brief Clears the audio data callback. If active, it stops invoking the
/// callback immediately.
/// This method should be called from the JS thread only.
void AudioRecorder::clearOnAudioReadyCallback() {
  std::scoped_lock lock(callbackMutex_);
  callbackOutputConfigured_.store(false, std::memory_order_release);
  callbackOutputEnabled_.store(false, std::memory_order_release);
  dataCallback_ = nullptr;
}

std::vector<std::string> AudioRecorder::buildOutputPaths(
    const std::vector<std::string> &segmentPaths,
    const std::string &filePath) {
  std::vector<std::string> outputPaths;

  for (const auto &raw : segmentPaths) {
    if (!raw.empty()) {
      outputPaths.push_back("file://" + raw);
    }
  }
  if (outputPaths.empty() && !filePath.empty()) {
    outputPaths.push_back("file://" + filePath);
  }

  return outputPaths;
}

AudioRecorder::StopResult AudioRecorder::finalizeStoppedOutputs(
    std::shared_ptr<AudioFileWriter> &fileWriter,
    std::shared_ptr<AudioRecorderCallback> &dataCallback,
    std::shared_ptr<RecorderAdapterNode> &adapterNode,
    std::vector<std::string> outputPaths) {
  double outputFileSize = 0.0;
  double outputDuration = 0.0;
  std::string closeFileError;

  if (fileWriter != nullptr) {
    auto fileResult = fileWriter->closeFile();

    if (!fileResult.is_ok()) {
      closeFileError = fileResult.unwrap_err();
    } else {
      outputFileSize = std::get<0>(fileResult.unwrap());
      outputDuration = std::get<1>(fileResult.unwrap());
    }

    fileWriter = nullptr;
  }

  if (dataCallback != nullptr) {
    dataCallback->cleanup();
    dataCallback = nullptr;
  }

  if (adapterNode != nullptr) {
    adapterNode->adapterCleanup();
    adapterNode = nullptr;
  }

  if (!closeFileError.empty()) {
    return StopResult::Err("Failed to close file: " + closeFileError);
  }

  return StopResult::Ok(std::make_tuple(std::move(outputPaths), outputFileSize, outputDuration));
}

} // namespace audioapi
