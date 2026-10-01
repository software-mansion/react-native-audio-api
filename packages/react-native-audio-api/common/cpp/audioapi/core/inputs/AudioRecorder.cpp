#include <audioapi/core/inputs/AudioRecorder.h>

#include <audioapi/core/sources/RecorderAdapterNode.h>
#include <audioapi/core/utils/AudioFileWriter.h>
#include <audioapi/core/utils/AudioRecorderCallback.h>
#include <audioapi/core/utils/Locker.h>
#include <audioapi/encoding/EncoderCapabilities.h>
#include <audioapi/utils/AudioFileProperties.h>

#include <memory>
#include <string>
#include <utility>

namespace audioapi {

void AudioRecorder::onAudioFrames(const float *const *channels, int numFrames) {
  if (channels == nullptr || numFrames <= 0) {
    return;
  }

  lastCallbackFrameCount_.store(numFrames, std::memory_order_release);

  if (usesFileOutput()) {
    auto fileWriterLock = Locker::tryLock(fileWriterMutex_);
    if (fileWriterLock && fileWriter_) {
      fileWriter_->writeAudioData(channels, numFrames);
    }
  }

  if (usesCallback()) {
    auto callbackLock = Locker::tryLock(callbackMutex_);
    if (callbackLock && dataCallback_) {
      dataCallback_->receiveAudioData(channels, numFrames);
    }
  }

  if (isConnected()) {
    auto adapterLock = Locker::tryLock(adapterNodeMutex_);
    if (!adapterLock) {
      return;
    }
    if (auto *adapterNode = adapterNodeOf(adapterNodeHandle_)) {
      adapterNode->writeFrames(channels, static_cast<size_t>(numFrames));
    }
  }
}

/// JS thread only. The file itself is created by the next start(). An
/// active (recording or paused) session keeps the output it started with, so calling this during
/// a session fails and changes nothing.
Result<NoneType, std::string> AudioRecorder::enableFileOutput(
    std::shared_ptr<AudioFileProperties> properties) {
  if (properties == nullptr) {
    return Err("File output requires file properties");
  }
  auto validationResult = properties->validate();
  if (validationResult.is_err()) {
    return validationResult;
  }
  auto outputSpecResult = encoder_capabilities::resolveOutputSpec(properties->encoding.format);
  if (outputSpecResult.is_err()) {
    return Err(outputSpecResult.unwrap_err());
  }

  std::scoped_lock fileWriterLock(fileWriterMutex_, errorCallbackMutex_);

  if (!isIdle()) {
    return Err("File output cannot be changed while a recording session is active");
  }

  fileProperties_ = std::move(properties);
  fileOutputState_.store(OutputState::Requested, std::memory_order_release);

  return Ok(None);
}

/// JS thread only. Closes the file immediately when called mid-recording.
void AudioRecorder::disableFileOutput() {
  std::shared_ptr<AudioFileWriter> fileWriter;

  {
    std::scoped_lock fileWriterLock(fileWriterMutex_);
    fileOutputState_.store(OutputState::Disabled, std::memory_order_release);
    fileWriter = std::move(fileWriter_);
  }

  if (fileWriter != nullptr) {
    fileWriter->closeFile();
  }
}

Result<NoneType, std::string> AudioRecorder::setupFileWriter(
    const std::shared_ptr<AudioFileProperties> &properties) {
  auto formatResult = resolveStreamFormat();

  if (!formatResult.is_ok()) {
    return Err("Failed to open file for writing: " + formatResult.unwrap_err());
  }

  fileWriter_ = std::make_shared<AudioFileWriter>(audioEventHandlerRegistry_, properties);
  fileWriter_->setOnErrorCallback(errorCallbackId_.load(std::memory_order_acquire));

  const auto format = formatResult.unwrap();
  auto fileResult = fileWriter_->openFile(format);

  if (!fileResult.is_ok()) {
    deactivate(fileOutputState_);
    fileWriter_ = nullptr;
    return Err("Failed to open file for writing: " + fileResult.unwrap_err());
  }

  fileOutputState_.store(OutputState::Active, std::memory_order_release);
  return Ok(None);
}

/// JS thread only. Prepares the callback immediately when called mid-recording.
Result<NoneType, std::string> AudioRecorder::setOnAudioReadyCallback(
    float sampleRate,
    size_t bufferLength,
    int channelCount,
    uint64_t callbackId) {
  std::scoped_lock callbackLock(callbackMutex_, errorCallbackMutex_);
  dataCallback_ = std::make_shared<AudioRecorderCallback>(
      audioEventHandlerRegistry_, sampleRate, bufferLength, channelCount, callbackId);
  dataCallback_->setOnErrorCallback(errorCallbackId_.load(std::memory_order_acquire));
  callbackOutputState_.store(OutputState::Requested, std::memory_order_release);

  if (isIdle()) {
    return Ok(None);
  }

  auto formatResult = resolveStreamFormat();

  // The input is unavailable only transiently, so keep the callback registered: the next
  // start() prepares it against whatever format the input comes back with.
  if (!formatResult.is_ok()) {
    return Err(formatResult.unwrap_err());
  }

  const auto format = formatResult.unwrap();
  auto prepareResult = dataCallback_->prepare(format);

  if (!prepareResult.is_ok()) {
    callbackOutputState_.store(OutputState::Disabled, std::memory_order_release);
    dataCallback_ = nullptr;
    return Err(prepareResult.unwrap_err());
  }

  callbackOutputState_.store(OutputState::Active, std::memory_order_release);
  return Ok(None);
}

/// JS thread only.
void AudioRecorder::clearOnAudioReadyCallback() {
  std::scoped_lock callbackLock(callbackMutex_);
  callbackOutputState_.store(OutputState::Disabled, std::memory_order_release);
  dataCallback_ = nullptr;
}

/// JS thread only. Prepares the node immediately when called mid-recording.
void AudioRecorder::connect(const std::shared_ptr<utils::graph::NodeHandle> &node) {
  std::scoped_lock adapterLock(adapterNodeMutex_);
  adapterNodeHandle_ = node;
  connectionState_.store(OutputState::Requested, std::memory_order_release);

  if (isIdle()) {
    return;
  }

  auto formatResult = resolveStreamFormat();

  if (!formatResult.is_ok()) {
    return;
  }

  prepareAdapterNode(formatResult.unwrap());
}

/// JS thread only.
void AudioRecorder::disconnect() {
  std::shared_ptr<utils::graph::NodeHandle> adapterNodeHandle;
  bool hadConnection = false;

  {
    std::scoped_lock adapterLock(adapterNodeMutex_);
    hadConnection = isConnected();
    connectionState_.store(OutputState::Disabled, std::memory_order_release);
    adapterNodeHandle = std::move(adapterNodeHandle_);
  }

  if (auto *adapterNode = hadConnection ? adapterNodeOf(adapterNodeHandle) : nullptr) {
    adapterNode->adapterCleanup();
  }
}

void AudioRecorder::prepareAdapterNode(const StreamFormat &format) {
  auto *adapterNode = adapterNodeOf(adapterNodeHandle_);
  if (adapterNode == nullptr) {
    return;
  }

  adapterNode->init(format);
  connectionState_.store(OutputState::Active, std::memory_order_release);
}

RecorderAdapterNode *AudioRecorder::adapterNodeOf(
    const std::shared_ptr<utils::graph::NodeHandle> &handle) {
  if (handle == nullptr || handle->audioNode == nullptr) {
    return nullptr;
  }
  // NOLINTBEGIN (cppcoreguidelines-pro-type-static-cast-downcast)
  return static_cast<RecorderAdapterNode *>(handle->audioNode->asAudioNode());
  // NOLINTEND (cppcoreguidelines-pro-type-static-cast-downcast)
}

AudioRecorder::DetachedSideEffects AudioRecorder::detachSideEffects() {
  DetachedSideEffects sideEffects;

  if (usesFileOutput()) {
    deactivate(fileOutputState_);
    sideEffects.fileWriter = std::move(fileWriter_);
  }

  if (usesCallback()) {
    deactivate(callbackOutputState_);
    // Kept registered rather than moved out, so a later start() can re-prepare it.
    sideEffects.dataCallback = dataCallback_;
  }

  if (isConnected()) {
    deactivate(connectionState_);
    sideEffects.adapterNodeHandle = std::move(adapterNodeHandle_);
  }

  return sideEffects;
}

Result<FileInfo, std::string> AudioRecorder::finalizeSideEffects(DetachedSideEffects sideEffects) {
  double outputFileSize = 0.0;
  double outputDuration = 0.0;

  if (sideEffects.fileWriter != nullptr) {
    auto fileResult = sideEffects.fileWriter->closeFile();

    if (!fileResult.is_ok()) {
      return Err("Failed to close file: " + fileResult.unwrap_err());
    }

    const auto &session = fileResult.unwrap();
    outputFileSize = session.sizeMB;
    outputDuration = session.durationSec;
    for (const auto &filePath : session.filePaths) {
      sideEffects.fileUris.push_back("file://" + filePath);
    }
  }

  if (sideEffects.dataCallback != nullptr) {
    sideEffects.dataCallback->cleanup();
  }

  if (auto *adapterNode = adapterNodeOf(sideEffects.adapterNodeHandle)) {
    adapterNode->adapterCleanup();
  }

  return Ok(
      FileInfo{
          .paths = std::move(sideEffects.fileUris),
          .size = outputFileSize,
          .duration = outputDuration,
      });
}

/// JS thread only.
void AudioRecorder::setOnErrorCallback(uint64_t callbackId) {
  std::scoped_lock lock(callbackMutex_, fileWriterMutex_, errorCallbackMutex_);

  if (usesFileOutput() && fileWriter_ != nullptr) {
    fileWriter_->setOnErrorCallback(callbackId);
  }

  if (usesCallback() && dataCallback_ != nullptr) {
    dataCallback_->setOnErrorCallback(callbackId);
  }

  errorCallbackId_.store(callbackId, std::memory_order_release);
}

/// JS thread only.
void AudioRecorder::clearOnErrorCallback() {
  std::scoped_lock lock(callbackMutex_, fileWriterMutex_, errorCallbackMutex_);

  if (usesFileOutput() && fileWriter_ != nullptr) {
    fileWriter_->clearOnErrorCallback();
  }

  if (usesCallback() && dataCallback_ != nullptr) {
    dataCallback_->clearOnErrorCallback();
  }

  errorCallbackId_.store(0, std::memory_order_release);
}

double AudioRecorder::getCurrentDuration() const {
  double duration = 0.0;

  if (usesFileOutput() && fileWriter_ != nullptr) {
    duration = fileWriter_->getCurrentDuration();
  }

  return duration;
}

RecorderState AudioRecorder::getState() const {
  if (isIdle()) {
    return RecorderState::Idle;
  }

  return isPaused() ? RecorderState::Paused : RecorderState::Recording;
}

bool AudioRecorder::usesCallback() const {
  return callbackOutputState_.load(std::memory_order_acquire) == OutputState::Active;
}

bool AudioRecorder::usesFileOutput() const {
  return fileOutputState_.load(std::memory_order_acquire) == OutputState::Active;
}

bool AudioRecorder::isConnected() const {
  return connectionState_.load(std::memory_order_acquire) == OutputState::Active;
}

bool AudioRecorder::wantsCallback() const {
  return callbackOutputState_.load(std::memory_order_acquire) != OutputState::Disabled;
}

bool AudioRecorder::wantsFileOutput() const {
  return fileOutputState_.load(std::memory_order_acquire) != OutputState::Disabled;
}

bool AudioRecorder::wantsConnection() const {
  return connectionState_.load(std::memory_order_acquire) != OutputState::Disabled;
}

void AudioRecorder::deactivate(std::atomic<OutputState> &state) {
  auto expected = OutputState::Active;
  state.compare_exchange_strong(expected, OutputState::Requested, std::memory_order_acq_rel);
}

} // namespace audioapi
