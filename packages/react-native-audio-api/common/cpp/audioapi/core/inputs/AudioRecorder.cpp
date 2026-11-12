#include <audioapi/core/inputs/AudioRecorder.h>

#include <audioapi/core/inputs/IRecorderInput.h>
#include <audioapi/core/inputs/IRecordingSessionRegistry.h>
#include <audioapi/core/inputs/RecorderInputEvents.h>
#include <audioapi/core/inputs/RecorderInputGate.h>
#include <audioapi/core/sources/RecorderAdapterNode.h>
#include <audioapi/core/utils/AudioFileWriter.h>
#include <audioapi/core/utils/AudioRecorderCallback.h>
#include <audioapi/core/utils/Locker.h>
#include <audioapi/events/IAudioEventHandlerRegistry.h>
#include <audioapi/utils/AudioFileProperties.h>

#include <memory>
#include <optional>
#include <string>
#include <utility>

namespace audioapi {

AudioRecorder::AudioRecorder(
    const std::shared_ptr<IAudioEventHandlerRegistry> &audioEventHandlerRegistry,
    std::shared_ptr<IRecorderInput> input,
    const PlatformFileBackend *fileBackend,
    IRecordingSessionRegistry *sessionRegistry)
    : input_(std::move(input)),
      inputGate_(std::make_shared<RecorderInputGate>(this)),
      audioEventHandlerRegistry_(audioEventHandlerRegistry),
      fileBackend_(fileBackend != nullptr ? *fileBackend : osFileBackend()),
      sessionRegistry_(sessionRegistry) {}

/// An idle recorder's input is already closed and its gate disarmed.
AudioRecorder::~AudioRecorder() {
  stop();
}

void AudioRecorder::onInputFrames(const float *const *channels, int channelCount, int numFrames) {
  // Frames in a layout the outputs were not prepared for: the platform reconfigured the input and
  // the recorder has not followed it yet.
  if (channels == nullptr || numFrames <= 0 ||
      channelCount != session_.format->layout.channelCount) {
    return;
  }

  session_.lastBufferFrames.store(numFrames, std::memory_order_release);

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

void AudioRecorder::enter(Phase phase) {
  if (phase != Phase::Recording) {
    inputGate_->disarmAndWait();
  }

  session_.phase.store(phase, std::memory_order_release);

  if (phase == Phase::Recording) {
    inputGate_->arm();
  }
}

Result<NoneType, std::string> AudioRecorder::start() {
  if (!isIdle()) {
    return Err("Recorder is already recording");
  }
  if (sessionRegistry_ != nullptr && !sessionRegistry_->claim(shared_from_this())) {
    return Err("Another recording is already in progress");
  }

  auto startResult = startClaimedSession();
  if (startResult.is_err() && sessionRegistry_ != nullptr) {
    sessionRegistry_->ended(*this, std::nullopt);
  }
  return startResult;
}

Result<NoneType, std::string> AudioRecorder::startClaimedSession() {
  // Taken before the control mutex: it can wait seconds for a Bluetooth link, and every other
  // control path would wait behind it.
  auto route = input_->acquireRoute();

  Result<NoneType, std::string> failure = Ok(None);
  DetachedSideEffects abandoned;

  {
    std::scoped_lock controlLock(controlMutex_);

    if (!isIdle()) {
      return Err("Recorder is already recording");
    }

    auto formatResult = input_->open(
        std::move(route),
        InputOpenPurpose::StartSession,
        inputGate_,
        RecorderInputEvents(weak_from_this(), session_.inputGeneration));
    if (formatResult.is_err()) {
      return Err(formatResult.unwrap_err());
    }

    session_.setFormat(formatResult.unwrap());

    {
      std::scoped_lock outputsLock(callback_.mutex, file_.mutex, adapter_.mutex);
      failure = prepareOutputs(*session_.format);
    }

    if (failure.is_ok()) {
      failure = input_->start();
    }

    if (failure.is_ok()) {
      enter(Phase::Recording);
      return Ok(None);
    }

    abandoned = endSession();
  }

  finalizeSideEffects(std::move(abandoned));
  return failure;
}

AudioRecorder::DetachedSideEffects AudioRecorder::endSession() {
  enter(Phase::Idle);
  input_->close();
  ++session_.inputGeneration;
  session_.setFormat(std::nullopt);

  std::scoped_lock outputsLock(callback_.mutex, file_.mutex, adapter_.mutex);
  return detachSideEffects();
}

Result<FileInfo, std::string> AudioRecorder::stop() {
  DetachedSideEffects sideEffects;

  {
    std::scoped_lock controlLock(controlMutex_);

    if (isIdle()) {
      return Err("Recorder is not in recording state.");
    }

    sideEffects = endSession();
  }

  auto recording = finalizeSideEffects(std::move(sideEffects));

  // Only now that the files are closed, so the session and its recording change hands at once.
  if (sessionRegistry_ != nullptr) {
    sessionRegistry_->ended(
        *this, recording.is_ok() ? std::optional<FileInfo>(recording.unwrap()) : std::nullopt);
  }
  return recording;
}

void AudioRecorder::pause() {
  std::scoped_lock controlLock(controlMutex_);

  switch (session_.phase.load(std::memory_order_acquire)) {
    case Phase::Recording:
      // Not when the system already stopped the input: resume() restarts that one.
      if (input_->isRunning()) {
        enter(Phase::Paused);
        input_->pause();
      }
      return;
    case Phase::Rerouting:
      session_.phaseAfterReroute.store(Phase::Paused, std::memory_order_release);
      return;
    case Phase::Idle:
    case Phase::Paused:
      return;
  }
}

Result<NoneType, std::string> AudioRecorder::resume() {
  std::scoped_lock controlLock(controlMutex_);

  switch (session_.phase.load(std::memory_order_acquire)) {
    case Phase::Idle:
      return Err("Recorder is not in recording state.");
    case Phase::Rerouting:
      // The reroute reopens a paused session's input stopped, so this resume would be lost.
      if (session_.phaseAfterReroute.load(std::memory_order_acquire) == Phase::Paused) {
        return Err("Cannot resume while the input stream is restarting");
      }
      return Ok(None);
    case Phase::Recording:
      if (input_->isRunning()) {
        return Ok(None);
      }
      // The system stopped the input under a recording session.
      break;
    case Phase::Paused:
      break;
  }

  // The input may come back in another format, so nothing is delivered until the outputs
  // follow it.
  const Phase phaseBefore = session_.phase.load(std::memory_order_acquire);
  enter(Phase::Paused);

  auto resumeResult = input_->resume();
  auto formatResult = input_->currentFormat();

  if (formatResult.is_ok()) {
    // Also after a refused resume: the call may still have reconfigured the input (iOS rebuilds
    // its graph before it tries to start it), and capture can come back later with no
    // notification.
    reportIfFailed(repointOutputsAt(formatResult.unwrap()));
  } else if (resumeResult.is_ok()) {
    // A running input whose format cannot be read would leave the session reading as recording
    // while nothing reaches the outputs, so the resume is undone instead.
    input_->pause();
  }

  const bool isCapturing = resumeResult.is_ok() && formatResult.is_ok();
  enter(isCapturing ? Phase::Recording : phaseBefore);

  if (resumeResult.is_err()) {
    return resumeResult;
  }
  if (formatResult.is_err()) {
    return Err("Recorder input format is unavailable after resuming: " + formatResult.unwrap_err());
  }
  return Ok(None);
}

void AudioRecorder::onInputFormatChanged(uint64_t openedGeneration) {
  std::scoped_lock controlLock(controlMutex_);

  if (isIdle() || openedGeneration != session_.inputGeneration) {
    return;
  }

  // Unreadable while the platform rebuilds its input graph. The gate is left as it is: arming it
  // would feed frames of an unknown format to the outputs, and disarming it would silence the
  // session for good if no later notification arrives.
  auto formatResult = input_->currentFormat();
  if (formatResult.is_ok()) {
    followInputFormat(formatResult.unwrap());
  }
}

void AudioRecorder::onInputFailed(uint64_t openedGeneration, const std::string &reason) {
  {
    std::scoped_lock controlLock(controlMutex_);
    if (isIdle() || openedGeneration != session_.inputGeneration) {
      return;
    }
  }

  reportError("Recorder input could not be recovered: " + reason);
}

void AudioRecorder::followInputFormat(const StreamFormat &format) {
  reportIfFailed(repointOutputsAt(format));

  // Repointing disarms the gate; entering the phase again arms it when recording.
  enter(session_.phase.load(std::memory_order_acquire));
}

Result<NoneType, std::string> AudioRecorder::rerouteInput(
    std::optional<uint64_t> openedGeneration) {
  auto reopenResult = reopenInput(openedGeneration);

  if (reopenResult.is_err()) {
    // Ended like any other session, so what was recorded so far is kept.
    stop();
    reportError("Recorder input could not be reopened: " + reopenResult.unwrap_err());
  }
  return reopenResult;
}

Result<NoneType, std::string> AudioRecorder::reopenInput(std::optional<uint64_t> openedGeneration) {
  uint64_t closedGeneration = 0;

  {
    std::scoped_lock controlLock(controlMutex_);

    const bool isStaleOpening =
        openedGeneration.has_value() && *openedGeneration != session_.inputGeneration;
    if (isIdle() || isStaleOpening) {
      return Ok(None);
    }

    // A reroute that arrives while another waits for its route takes over from it.
    const Phase phase = session_.phase.load(std::memory_order_acquire);
    if (phase != Phase::Rerouting) {
      session_.phaseAfterReroute.store(phase, std::memory_order_release);
      enter(Phase::Rerouting);
    }

    input_->close();
    closedGeneration = ++session_.inputGeneration;
  }

  // With the control mutex released, for the same reason as in start().
  auto route = input_->acquireRoute();

  std::scoped_lock controlLock(controlMutex_);

  // Stopped, or taken over by a later reroute, while the route was on its way.
  if (session_.phase.load(std::memory_order_acquire) != Phase::Rerouting ||
      session_.inputGeneration != closedGeneration) {
    return Ok(None);
  }

  auto formatResult = input_->open(
      std::move(route),
      InputOpenPurpose::ContinueSession,
      inputGate_,
      RecorderInputEvents(weak_from_this(), closedGeneration));
  if (formatResult.is_err()) {
    return Err(formatResult.unwrap_err());
  }

  auto outputsResult = repointOutputsAt(formatResult.unwrap());
  if (outputsResult.is_err()) {
    return outputsResult;
  }

  const Phase phaseToRestore = session_.phaseAfterReroute.load(std::memory_order_acquire);
  if (phaseToRestore == Phase::Recording) {
    auto startResult = input_->start();
    if (startResult.is_err()) {
      return startResult;
    }
  }

  enter(phaseToRestore);
  return Ok(None);
}

Result<NoneType, std::string> AudioRecorder::repointOutputsAt(const StreamFormat &format) {
  if (format == *session_.format) {
    return Ok(None);
  }

  inputGate_->disarmAndWait();
  session_.setFormat(format);

  std::scoped_lock outputsLock(callback_.mutex, file_.mutex, adapter_.mutex);
  return reprepareOutputs(format);
}

void AudioRecorder::SessionState::setFormat(const std::optional<StreamFormat> &newFormat) {
  format = newFormat;
  sampleRate.store(
      newFormat.has_value() ? newFormat->layout.sampleRate : 0.0F, std::memory_order_release);
  lastBufferFrames.store(0, std::memory_order_release);
}

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

  std::scoped_lock lock(controlMutex_, file_.mutex);

  if (!isIdle()) {
    return Err("File output cannot be changed while a recording session is active");
  }

  fileProperties_ = std::move(properties);
  file_.set(OutputState::Requested);

  return Ok(None);
}

void AudioRecorder::disableFileOutput() {
  std::shared_ptr<AudioFileWriter> fileWriter;

  {
    std::scoped_lock lock(controlMutex_, file_.mutex);
    file_.set(OutputState::Disabled);
    fileWriter = std::move(file_.target);
  }

  if (fileWriter != nullptr) {
    fileWriter->closeFile();
  }
}

Result<NoneType, std::string> AudioRecorder::setupFileWriter(
    const std::shared_ptr<AudioFileProperties> &properties) {
  file_.target =
      std::make_shared<AudioFileWriter>(audioEventHandlerRegistry_, properties, fileBackend_);
  file_.target->setOnErrorCallback(errorCallbackId_.load(std::memory_order_acquire));

  auto fileResult = file_.target->openFile(*session_.format);

  if (!fileResult.is_ok()) {
    file_.deactivate();
    file_.target = nullptr;
    return Err("Failed to open file for writing: " + fileResult.unwrap_err());
  }

  file_.set(OutputState::Active);
  return Ok(None);
}

Result<NoneType, std::string> AudioRecorder::setOnAudioReadyCallback(
    float sampleRate,
    size_t bufferLength,
    int channelCount,
    uint64_t callbackId) {
  std::scoped_lock lock(controlMutex_, callback_.mutex);
  callback_.target = std::make_shared<AudioRecorderCallback>(
      audioEventHandlerRegistry_, sampleRate, bufferLength, channelCount, callbackId);
  callback_.target->setOnErrorCallback(errorCallbackId_.load(std::memory_order_acquire));
  callback_.set(OutputState::Requested);

  if (isIdle()) {
    return Ok(None);
  }

  auto prepareResult = callback_.target->prepare(*session_.format);

  if (!prepareResult.is_ok()) {
    callback_.set(OutputState::Disabled);
    callback_.target = nullptr;
    return Err(prepareResult.unwrap_err());
  }

  callback_.set(OutputState::Active);
  return Ok(None);
}

void AudioRecorder::clearOnAudioReadyCallback() {
  std::scoped_lock lock(controlMutex_, callback_.mutex);
  callback_.set(OutputState::Disabled);
  callback_.target = nullptr;
}

void AudioRecorder::connect(const std::shared_ptr<utils::graph::NodeHandle> &node) {
  std::scoped_lock lock(controlMutex_, adapter_.mutex);
  adapter_.target = node;
  adapter_.set(OutputState::Requested);

  if (isIdle()) {
    return;
  }

  prepareAdapterNode(*session_.format);
}

void AudioRecorder::disconnect() {
  std::shared_ptr<utils::graph::NodeHandle> adapterNodeHandle;
  bool hadConnection = false;

  {
    std::scoped_lock lock(controlMutex_, adapter_.mutex);
    hadConnection = adapter_.isActive();
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
    auto writerResult = setupFileWriter(fileProperties_);
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

  if (file_.isActive()) {
    noteFailure(reprepareFileWriter(format));
  }
  if (callback_.isActive()) {
    noteFailure(prepareCallback(format));
  }
  if (adapter_.isActive()) {
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
  if (callback_.isActive()) {
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

  if (file_.isActive()) {
    file_.deactivate();
    sideEffects.fileWriter = std::move(file_.target);
  }

  if (callback_.isActive()) {
    callback_.deactivate();
    // Kept registered rather than moved out, so a later start() can re-prepare it.
    sideEffects.dataCallback = callback_.target;
  }

  if (adapter_.isActive()) {
    adapter_.deactivate();
    // Kept connected, like the callback, so a later start() feeds the same node again.
    sideEffects.adapterNodeHandle = adapter_.target;
  }

  return sideEffects;
}

Result<FileInfo, std::string> AudioRecorder::finalizeSideEffects(DetachedSideEffects sideEffects) {
  double outputFileSize = 0.0;
  double outputDuration = 0.0;
  std::optional<std::string> closeError;

  if (sideEffects.fileWriter != nullptr) {
    auto fileResult = sideEffects.fileWriter->closeFile();

    if (fileResult.is_ok()) {
      const auto &session = fileResult.unwrap();
      outputFileSize = session.sizeMB;
      outputDuration = session.durationSec;
      for (const auto &filePath : session.filePaths) {
        sideEffects.fileUris.push_back("file://" + filePath);
      }
    } else {
      closeError = "Failed to close file: " + fileResult.unwrap_err();
    }
  }

  if (sideEffects.dataCallback != nullptr) {
    sideEffects.dataCallback->cleanup();
  }

  if (auto *adapterNode = adapterNodeOf(sideEffects.adapterNodeHandle)) {
    adapterNode->adapterCleanup();
  }

  // Reported only now: the other outputs are torn down whether or not the file closed.
  if (closeError.has_value()) {
    return Err(*closeError);
  }

  return Ok(
      FileInfo{
          .paths = std::move(sideEffects.fileUris),
          .size = outputFileSize,
          .duration = outputDuration,
      });
}

void AudioRecorder::setOnErrorCallback(uint64_t callbackId) {
  std::scoped_lock lock(controlMutex_, callback_.mutex, file_.mutex);

  if (file_.isActive() && file_.target != nullptr) {
    file_.target->setOnErrorCallback(callbackId);
  }

  if (callback_.isActive() && callback_.target != nullptr) {
    callback_.target->setOnErrorCallback(callbackId);
  }

  errorCallbackId_.store(callbackId, std::memory_order_release);
}

void AudioRecorder::clearOnErrorCallback() {
  std::scoped_lock lock(controlMutex_, callback_.mutex, file_.mutex);

  if (file_.isActive() && file_.target != nullptr) {
    file_.target->clearOnErrorCallback();
  }

  if (callback_.isActive() && callback_.target != nullptr) {
    callback_.target->clearOnErrorCallback();
  }

  errorCallbackId_.store(0, std::memory_order_release);
}

/// Locked because stop() and disableFileOutput() reset file_.target; the lock is held only while
/// the writer pointer is swapped, never across a file close.
double AudioRecorder::getCurrentDuration() const {
  std::scoped_lock fileWriterLock(file_.mutex);

  if (file_.isActive() && file_.target != nullptr) {
    return file_.target->getCurrentDuration();
  }

  return 0.0;
}

double AudioRecorder::getInputLatency() const {
  if (isIdle()) {
    return 0.0;
  }

  const int32_t callbackFrames = session_.lastBufferFrames.load(std::memory_order_acquire);
  const float sampleRate = session_.sampleRate.load(std::memory_order_acquire);
  const double callbackSeconds = callbackFrames > 0 && sampleRate > 0.0F
      ? static_cast<double>(callbackFrames) / static_cast<double>(sampleRate)
      : 0.0;

  return input_->latency(callbackSeconds);
}

void AudioRecorder::reportIfFailed(const Result<NoneType, std::string> &result) {
  if (result.is_err()) {
    reportError(result.unwrap_err());
  }
}

void AudioRecorder::reportError(const std::string &message) {
  const uint64_t callbackId = errorCallbackId_.load(std::memory_order_acquire);

  if (audioEventHandlerRegistry_ == nullptr || callbackId == 0) {
    return;
  }

  audioEventHandlerRegistry_->dispatchEvent(
      AudioEvent::RECORDER_ERROR, callbackId, StringPayload{.name = "message", .reason = message});
}

bool AudioRecorder::isIdle() const {
  return session_.phase.load(std::memory_order_acquire) == Phase::Idle;
}

bool AudioRecorder::isRecording() const {
  return getState() == RecorderState::Recording;
}

bool AudioRecorder::isPaused() const {
  return getState() == RecorderState::Paused;
}

RecorderState AudioRecorder::getState() const {
  switch (session_.phase.load(std::memory_order_acquire)) {
    case Phase::Idle:
      return RecorderState::Idle;
    case Phase::Paused:
      return RecorderState::Paused;
    case Phase::Recording:
      // An input the system stopped (an interrupted iOS session) reads as paused.
      return input_->isRunning() ? RecorderState::Recording : RecorderState::Paused;
    case Phase::Rerouting:
      // Reads as what the session comes back as once the input is reopened.
      return session_.phaseAfterReroute.load(std::memory_order_acquire) == Phase::Paused
          ? RecorderState::Paused
          : RecorderState::Recording;
  }
  return RecorderState::Idle;
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
