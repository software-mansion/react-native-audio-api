#include <audioapi/core/inputs/RecorderOutputs.h>

#include <audioapi/core/sources/RecorderAdapterNode.h>
#include <audioapi/core/utils/AudioFileWriter.h>
#include <audioapi/core/utils/AudioRecorderCallback.h>
#include <audioapi/core/utils/Locker.h>
#include <audioapi/utils/AudioFileProperties.h>

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace audioapi {

RecorderOutputs::RecorderOutputs(
    const std::shared_ptr<IAudioEventHandlerRegistry> &audioEventHandlerRegistry,
    const PlatformFileBackend &fileBackend)
    : audioEventHandlerRegistry_(audioEventHandlerRegistry), fileBackend_(fileBackend) {}

void RecorderOutputs::write(const float *const *channels, int numFrames) {
  if (file_.isActive()) {
    auto fileWriterLock = Locker::tryLock(file_.mutex);
    if (fileWriterLock && file_.target) {
      file_.target->writeAudioData(channels, numFrames);
    }
  }

  if (callback_.isActive()) {
    auto callbackLock = Locker::tryLock(callback_.mutex);
    if (callbackLock && callback_.target) {
      callback_.target->receiveAudioData(channels, numFrames);
    }
  }

  if (adapter_.isActive()) {
    auto adapterLock = Locker::tryLock(adapter_.mutex);
    if (!adapterLock) {
      return;
    }
    if (auto *adapterNode = adapterNodeOf(adapter_.target)) {
      adapterNode->writeFrames(channels, static_cast<size_t>(numFrames));
    }
  }
}

Result<NoneType, std::string> RecorderOutputs::prepare(const StreamFormat &format) {
  std::scoped_lock outputsLock(callback_.mutex, file_.mutex, adapter_.mutex);

  if (file_.isWanted()) {
    auto writerResult = prepareFileWriter(format);
    if (writerResult.is_err()) {
      return writerResult;
    }
  }

  if (callback_.isWanted()) {
    auto callbackResult = prepareCallback(format);
    if (callbackResult.is_err()) {
      return callbackResult;
    }
  }

  if (adapter_.isWanted()) {
    return prepareAdapterNode(format);
  }

  return Ok(None);
}

Result<NoneType, std::string> RecorderOutputs::follow(const StreamFormat &format) {
  std::scoped_lock outputsLock(callback_.mutex, file_.mutex, adapter_.mutex);

  std::string failures;
  const auto noteFailure = [&failures](const Result<NoneType, std::string> &result) {
    if (result.is_err()) {
      failures += (failures.empty() ? "" : "; ") + result.unwrap_err();
    }
  };

  if (file_.isActive()) {
    noteFailure(reprepareFileWriter(format));
  }
  if (callback_.isActive()) {
    noteFailure(prepareCallback(format));
  }
  if (adapter_.isActive()) {
    if (auto *adapterNode = adapterNodeOf(adapter_.target)) {
      adapterNode->adapterCleanup();
    }
    noteFailure(prepareAdapterNode(format));
  }

  if (failures.empty()) {
    return Ok(None);
  }
  return Err(failures);
}

RecorderOutputs::Detached RecorderOutputs::detach() {
  std::scoped_lock outputsLock(callback_.mutex, file_.mutex, adapter_.mutex);
  Detached detached;

  // moved because file writer belongs to only one session
  file_.deactivate();
  detached.fileWriter = std::move(file_.target);

  callback_.deactivate();
  detached.dataCallback = callback_.target;

  adapter_.deactivate();
  detached.adapterNodeHandle = adapter_.target;

  return detached;
}

Result<FileInfo, std::string> RecorderOutputs::finalize(Detached detached) {
  double outputFileSize = 0.0;
  double outputDuration = 0.0;
  std::vector<std::string> fileUris;
  std::optional<std::string> closeError;

  if (detached.fileWriter != nullptr) {
    auto fileResult = detached.fileWriter->closeFile();

    if (fileResult.is_ok()) {
      const auto &session = fileResult.unwrap();
      outputFileSize = session.sizeMB;
      outputDuration = session.durationSec;
      for (const auto &filePath : session.filePaths) {
        fileUris.push_back("file://" + filePath);
      }
    } else {
      closeError = "Failed to close file: " + fileResult.unwrap_err();
    }
  }

  if (detached.dataCallback != nullptr) {
    detached.dataCallback->cleanup();
  }

  if (auto *adapterNode = adapterNodeOf(detached.adapterNodeHandle)) {
    adapterNode->adapterCleanup();
  }

  if (closeError.has_value()) {
    return Err(*closeError);
  }

  return Ok(
      FileInfo{
          .paths = std::move(fileUris),
          .size = outputFileSize,
          .duration = outputDuration,
      });
}

Result<NoneType, std::string> RecorderOutputs::enableFile(
    std::shared_ptr<AudioFileProperties> properties) {
  if (properties == nullptr) {
    return Err("File output requires file properties");
  }
  auto validationResult = properties->validate();
  if (validationResult.is_err()) {
    return validationResult;
  }
  auto outputSpecResult = fileBackend_.resolveOutputSpec(properties->encoding.format);
  if (outputSpecResult.is_err()) {
    return Err(outputSpecResult.unwrap_err());
  }

  std::scoped_lock fileLock(file_.mutex);
  fileProperties_ = std::move(properties);
  file_.set(OutputState::Requested);
  return Ok(None);
}

RecorderOutputs::Detached RecorderOutputs::disableFile() {
  std::scoped_lock fileLock(file_.mutex);
  file_.set(OutputState::Disabled);
  return Detached{.fileWriter = std::move(file_.target)};
}

void RecorderOutputs::connect(
    const std::shared_ptr<utils::graph::NodeHandle> &node,
    const std::optional<StreamFormat> &currentFormat) {
  std::scoped_lock adapterLock(adapter_.mutex);
  adapter_.target = node;
  adapter_.set(OutputState::Requested);

  if (currentFormat.has_value()) {
    prepareAdapterNode(*currentFormat);
  }
}

RecorderOutputs::Detached RecorderOutputs::disconnect() {
  std::scoped_lock adapterLock(adapter_.mutex);
  const bool wasPrepared = adapter_.isActive();
  adapter_.set(OutputState::Disabled);

  auto adapterNodeHandle = std::move(adapter_.target);
  if (!wasPrepared) {
    return {};
  }
  return Detached{.adapterNodeHandle = std::move(adapterNodeHandle)};
}

Result<NoneType, std::string> RecorderOutputs::setCallback(
    float sampleRate,
    size_t bufferLength,
    int channelCount,
    uint64_t callbackId,
    const std::optional<StreamFormat> &currentFormat) {
  std::scoped_lock callbackLock(callback_.mutex);
  callback_.target = std::make_shared<AudioRecorderCallback>(
      audioEventHandlerRegistry_, sampleRate, bufferLength, channelCount, callbackId);
  callback_.set(OutputState::Requested);

  if (!currentFormat.has_value()) {
    return Ok(None);
  }

  auto prepareResult = prepareCallback(*currentFormat);
  if (prepareResult.is_err()) {
    callback_.set(OutputState::Disabled);
    callback_.target = nullptr;
  }
  return prepareResult;
}

void RecorderOutputs::clearCallback() {
  std::scoped_lock callbackLock(callback_.mutex);
  callback_.set(OutputState::Disabled);
  callback_.target = nullptr;
}

void RecorderOutputs::setErrorCallbackId(uint64_t callbackId) {
  std::scoped_lock outputsLock(callback_.mutex, file_.mutex);

  // The ones not prepared yet take it up when they are.
  if (file_.isActive() && file_.target != nullptr) {
    file_.target->setOnErrorCallback(callbackId);
  }
  if (callback_.isActive() && callback_.target != nullptr) {
    callback_.target->setOnErrorCallback(callbackId);
  }

  errorCallbackId_.store(callbackId, std::memory_order_release);
}

uint64_t RecorderOutputs::errorCallbackId() const {
  return errorCallbackId_.load(std::memory_order_acquire);
}

/// Locked because detach() and disableFile() take file_.target away; the lock is held only while
/// the writer pointer is swapped, never across a file close.
double RecorderOutputs::currentFileDuration() const {
  std::scoped_lock fileLock(file_.mutex);

  if (file_.isActive() && file_.target != nullptr) {
    return file_.target->getCurrentDuration();
  }

  return 0.0;
}

Result<NoneType, std::string> RecorderOutputs::prepareFileWriter(const StreamFormat &format) {
  file_.target =
      std::make_shared<AudioFileWriter>(audioEventHandlerRegistry_, fileProperties_, fileBackend_);
  file_.target->setOnErrorCallback(errorCallbackId());

  auto fileResult = file_.target->openFile(format);

  if (fileResult.is_err()) {
    file_.deactivate();
    file_.target = nullptr;
    return Err("Failed to open file for writing: " + fileResult.unwrap_err());
  }

  file_.set(OutputState::Active);
  return Ok(None);
}

Result<NoneType, std::string> RecorderOutputs::reprepareFileWriter(const StreamFormat &format) {
  if (file_.target == nullptr) {
    return Err("File writer is unavailable");
  }

  auto result = file_.target->reprepareStreamFormat(format);
  if (result.is_err()) {
    file_.deactivate();
    return Err("Failed to continue the recording in the new input format: " + result.unwrap_err());
  }

  file_.set(OutputState::Active);
  return Ok(None);
}

Result<NoneType, std::string> RecorderOutputs::prepareCallback(const StreamFormat &format) {
  if (callback_.target == nullptr) {
    return Err("Callback output is unavailable.");
  }
  if (callback_.isActive()) {
    callback_.target->cleanup();
  }

  callback_.target->setOnErrorCallback(errorCallbackId());
  auto callbackResult = callback_.target->prepare(format);

  if (callbackResult.is_err()) {
    callback_.deactivate();
    return Err("Failed to prepare callback: " + callbackResult.unwrap_err());
  }

  callback_.set(OutputState::Active);
  return Ok(None);
}

Result<NoneType, std::string> RecorderOutputs::prepareAdapterNode(const StreamFormat &format) {
  auto *adapterNode = adapterNodeOf(adapter_.target);
  if (adapterNode == nullptr) {
    adapter_.deactivate();
    return Err("Recorder adapter node is unavailable.");
  }

  adapterNode->init(format);
  adapter_.set(OutputState::Active);
  return Ok(None);
}

RecorderAdapterNode *RecorderOutputs::adapterNodeOf(
    const std::shared_ptr<utils::graph::NodeHandle> &handle) {
  if (handle == nullptr || handle->audioNode == nullptr) {
    return nullptr;
  }
  // NOLINTBEGIN (cppcoreguidelines-pro-type-static-cast-downcast)
  return static_cast<RecorderAdapterNode *>(handle->audioNode->asAudioNode());
  // NOLINTEND (cppcoreguidelines-pro-type-static-cast-downcast)
}

} // namespace audioapi
