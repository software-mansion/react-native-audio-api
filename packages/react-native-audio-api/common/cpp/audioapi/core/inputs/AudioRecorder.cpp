#include <audioapi/core/inputs/AudioRecorder.h>

#include <audioapi/core/sources/RecorderAdapterNode.h>
#include <audioapi/core/utils/AudioFileWriter.h>
#include <audioapi/core/utils/AudioRecorderCallback.h>
#include <audioapi/core/utils/Locker.h>
#include <audioapi/events/IAudioEventHandlerRegistry.h>
#include <audioapi/utils/AudioFileProperties.h>

#include <memory>
#include <string>
#include <utility>

namespace audioapi {

AudioRecorder::AudioRecorder(
    const std::shared_ptr<IAudioEventHandlerRegistry> &audioEventHandlerRegistry,
    const PlatformFileBackend *fileBackend)
    : audioEventHandlerRegistry_(audioEventHandlerRegistry),
      fileBackend_(fileBackend != nullptr ? *fileBackend : osFileBackend()) {}

AudioRecorder::~AudioRecorder() {
  DetachedSideEffects leftovers;
  {
    std::scoped_lock outputsLock(callback_.mutex, file_.mutex, adapter_.mutex);
    leftovers = detachSideEffects();
  }
  finalizeSideEffects(std::move(leftovers));
}

void AudioRecorder::onAudioFrames(const float *const *channels, int numFrames) {
  if (channels == nullptr || numFrames <= 0) {
    return;
  }

  lastCallbackFrameCount_.store(numFrames, std::memory_order_release);

  if (usesFileOutput()) {
    auto fileWriterLock = Locker::tryLock(file_.mutex);
    if (fileWriterLock && file_.target) {
      file_.target->writeAudioData(channels, numFrames);
    }
  }

  if (usesCallback()) {
    auto callbackLock = Locker::tryLock(callback_.mutex);
    if (callbackLock && callback_.target) {
      callback_.target->receiveAudioData(channels, numFrames);
    }
  }

  if (isConnected()) {
    auto adapterLock = Locker::tryLock(adapter_.mutex);
    if (!adapterLock) {
      return;
    }
    if (auto *adapterNode = adapterNodeOf(adapter_.target)) {
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
  auto outputSpecResult = fileBackend_.resolveOutputSpec(properties->encoding.format);
  if (outputSpecResult.is_err()) {
    return Err(outputSpecResult.unwrap_err());
  }

  std::scoped_lock fileWriterLock(file_.mutex, errorCallbackMutex_);

  if (!isIdle()) {
    return Err("File output cannot be changed while a recording session is active");
  }

  fileProperties_ = std::move(properties);
  file_.set(OutputState::Requested);

  return Ok(None);
}

/// Control lane only. Closes the file immediately when called mid-recording, which joins
/// the writer thread; that is why it is queued behind start()/stop() instead of run on JS.
void AudioRecorder::disableFileOutput() {
  std::shared_ptr<AudioFileWriter> fileWriter;

  {
    std::scoped_lock fileWriterLock(file_.mutex);
    file_.set(OutputState::Disabled);
    fileWriter = std::move(file_.target);
  }

  if (fileWriter != nullptr) {
    fileWriter->closeFile();
  }
}

Result<NoneType, std::string> AudioRecorder::setupFileWriter(
    const std::shared_ptr<AudioFileProperties> &properties,
    const StreamFormat &format) {
  file_.target =
      std::make_shared<AudioFileWriter>(audioEventHandlerRegistry_, properties, fileBackend_);
  file_.target->setOnErrorCallback(errorCallbackId_.load(std::memory_order_acquire));

  auto fileResult = file_.target->openFile(format);

  if (!fileResult.is_ok()) {
    file_.deactivate();
    file_.target = nullptr;
    return Err("Failed to open file for writing: " + fileResult.unwrap_err());
  }

  file_.set(OutputState::Active);
  return Ok(None);
}

/// JS thread only. Prepares the callback immediately when called mid-recording.
Result<NoneType, std::string> AudioRecorder::setOnAudioReadyCallback(
    float sampleRate,
    size_t bufferLength,
    int channelCount,
    uint64_t callbackId) {
  std::scoped_lock callbackLock(callback_.mutex, errorCallbackMutex_);
  callback_.target = std::make_shared<AudioRecorderCallback>(
      audioEventHandlerRegistry_, sampleRate, bufferLength, channelCount, callbackId);
  callback_.target->setOnErrorCallback(errorCallbackId_.load(std::memory_order_acquire));
  callback_.set(OutputState::Requested);

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
  auto prepareResult = callback_.target->prepare(format);

  if (!prepareResult.is_ok()) {
    callback_.set(OutputState::Disabled);
    callback_.target = nullptr;
    return Err(prepareResult.unwrap_err());
  }

  callback_.set(OutputState::Active);
  return Ok(None);
}

/// JS thread only.
void AudioRecorder::clearOnAudioReadyCallback() {
  std::scoped_lock callbackLock(callback_.mutex);
  callback_.set(OutputState::Disabled);
  callback_.target = nullptr;
}

/// JS thread only. Prepares the node immediately when called mid-recording.
void AudioRecorder::connect(const std::shared_ptr<utils::graph::NodeHandle> &node) {
  std::scoped_lock adapterLock(adapter_.mutex);
  adapter_.target = node;
  adapter_.set(OutputState::Requested);

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
    std::scoped_lock adapterLock(adapter_.mutex);
    hadConnection = isConnected();
    adapter_.set(OutputState::Disabled);
    adapterNodeHandle = std::move(adapter_.target);
  }

  if (auto *adapterNode = hadConnection ? adapterNodeOf(adapterNodeHandle) : nullptr) {
    adapterNode->adapterCleanup();
  }
}

Result<NoneType, std::string> AudioRecorder::prepareAdapterNode(const StreamFormat &format) {
  auto *adapterNode = adapterNodeOf(adapter_.target);
  if (adapterNode == nullptr) {
    adapter_.deactivate();
    return Err("Recorder adapter node is unavailable.");
  }

  adapterNode->init(format);
  adapter_.set(OutputState::Active);
  return Ok(None);
}

Result<NoneType, std::string> AudioRecorder::prepareOutputs(const StreamFormat &format) {
  if (file_.isWanted()) {
    auto writerResult = setupFileWriter(fileProperties_, format);
    if (!writerResult.is_ok()) {
      return writerResult;
    }
  }

  if (callback_.isWanted()) {
    auto callbackResult = prepareCallback(format);
    if (!callbackResult.is_ok()) {
      return callbackResult;
    }
  }

  if (adapter_.isWanted()) {
    auto adapterResult = prepareAdapterNode(format);
    if (!adapterResult.is_ok()) {
      return adapterResult;
    }
  }

  return Ok(None);
}

Result<NoneType, std::string> AudioRecorder::reprepareOutputs(const StreamFormat &format) {
  std::string failures;
  const auto noteFailure = [&failures](const Result<NoneType, std::string> &result) {
    if (result.is_err()) {
      failures += (failures.empty() ? "" : "; ") + result.unwrap_err();
    }
  };

  if (usesFileOutput()) {
    noteFailure(reprepareFileWriter(format));
  }
  if (usesCallback()) {
    noteFailure(prepareCallback(format));
  }
  if (isConnected()) {
    noteFailure(reprepareAdapterNode(format));
  }

  if (failures.empty()) {
    return Ok(None);
  }
  return Err(failures);
}

Result<NoneType, std::string> AudioRecorder::prepareCallback(const StreamFormat &format) {
  if (callback_.target == nullptr) {
    return Err("Callback output is unavailable.");
  }
  if (usesCallback()) {
    callback_.target->cleanup();
  }

  callback_.target->setOnErrorCallback(errorCallbackId_.load(std::memory_order_acquire));
  auto callbackResult = callback_.target->prepare(format);

  if (!callbackResult.is_ok()) {
    callback_.deactivate();
    return Err("Failed to prepare callback: " + callbackResult.unwrap_err());
  }

  callback_.set(OutputState::Active);
  return Ok(None);
}

Result<NoneType, std::string> AudioRecorder::reprepareFileWriter(const StreamFormat &format) {
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

Result<NoneType, std::string> AudioRecorder::reprepareAdapterNode(const StreamFormat &format) {
  // init() is a no-op on an initialized adapter, so the old format has to be torn down first.
  if (auto *adapterNode = adapterNodeOf(adapter_.target)) {
    adapterNode->adapterCleanup();
  }
  return prepareAdapterNode(format);
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

  // Every output in any state: one that is not active has nothing to undo, and the final steps
  // are safe on an output that was never prepared. A writer that could not follow a format
  // change is no longer active but still holds this session's finished file.
  file_.deactivate();
  sideEffects.fileWriter = std::move(file_.target);

  // The callback and the adapter node are copied rather than moved out: they stay registered,
  // so the next start() prepares the same ones again.
  callback_.deactivate();
  sideEffects.dataCallback = callback_.target;

  adapter_.deactivate();
  sideEffects.adapterNodeHandle = adapter_.target;

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
  std::scoped_lock lock(callback_.mutex, file_.mutex, errorCallbackMutex_);

  if (usesFileOutput() && file_.target != nullptr) {
    file_.target->setOnErrorCallback(callbackId);
  }

  if (usesCallback() && callback_.target != nullptr) {
    callback_.target->setOnErrorCallback(callbackId);
  }

  errorCallbackId_.store(callbackId, std::memory_order_release);
}

/// JS thread only.
void AudioRecorder::clearOnErrorCallback() {
  std::scoped_lock lock(callback_.mutex, file_.mutex, errorCallbackMutex_);

  if (usesFileOutput() && file_.target != nullptr) {
    file_.target->clearOnErrorCallback();
  }

  if (usesCallback() && callback_.target != nullptr) {
    callback_.target->clearOnErrorCallback();
  }

  errorCallbackId_.store(0, std::memory_order_release);
}

/// JS thread. Locked because stop() and disableFileOutput() reset the writer from the control
/// lane; the lock is held only while the writer pointer is swapped, never across a file close.
double AudioRecorder::getCurrentDuration() const {
  std::scoped_lock fileWriterLock(file_.mutex);

  if (usesFileOutput() && file_.target != nullptr) {
    return file_.target->getCurrentDuration();
  }

  return 0.0;
}

double AudioRecorder::getInputLatency() const {
  if (isIdle()) {
    return 0.0;
  }

  const int32_t callbackFrames = lastCallbackFrameCount_.load(std::memory_order_acquire);
  const float sampleRate = streamSampleRate_.load(std::memory_order_acquire);
  const double callbackSeconds = callbackFrames > 0 && sampleRate > 0.0F
      ? static_cast<double>(callbackFrames) / static_cast<double>(sampleRate)
      : 0.0;

  return platformInputLatency(callbackSeconds);
}

void AudioRecorder::reportError(const std::string &message) {
  const uint64_t callbackId = errorCallbackId_.load(std::memory_order_acquire);

  if (audioEventHandlerRegistry_ == nullptr || callbackId == 0) {
    return;
  }

  audioEventHandlerRegistry_->dispatchEvent(
      AudioEvent::RECORDER_ERROR, callbackId, StringPayload{.name = "message", .reason = message});
}

RecorderState AudioRecorder::getState() const {
  if (isIdle()) {
    return RecorderState::Idle;
  }

  return isPaused() ? RecorderState::Paused : RecorderState::Recording;
}

bool AudioRecorder::usesCallback() const {
  return callback_.isActive();
}

bool AudioRecorder::usesFileOutput() const {
  return file_.isActive();
}

bool AudioRecorder::isConnected() const {
  return adapter_.isActive();
}

} // namespace audioapi
