#include <audioapi/core/inputs/AudioRecorder.h>

#include <audioapi/core/inputs/ActiveRecorderHandle.h>
#include <audioapi/core/inputs/IRecorderInput.h>
#include <audioapi/core/inputs/RecorderInputEvents.h>
#include <audioapi/core/inputs/RecorderInputGate.h>
#include <audioapi/core/utils/AudioFileWriter.h>
#include <audioapi/events/IAudioEventHandlerRegistry.h>

#include <memory>
#include <optional>
#include <string>
#include <utility>

namespace audioapi {

AudioRecorder::AudioRecorder(
    const std::shared_ptr<IAudioEventHandlerRegistry> &audioEventHandlerRegistry,
    std::shared_ptr<IRecorderInput> input,
    const PlatformFileBackend *fileBackend)
    : input_(std::move(input)),
      inputGate_(std::make_shared<RecorderInputGate>(this)),
      outputs_(audioEventHandlerRegistry, fileBackend != nullptr ? *fileBackend : osFileBackend()),
      audioEventHandlerRegistry_(audioEventHandlerRegistry) {}

/// The platform input can outlive the recorder and keeps calling the gate, which holds a raw
/// pointer to it: disarm before any teardown, whatever phase the recorder is in.
AudioRecorder::~AudioRecorder() {
  inputGate_->disarmAndWait();
  stop();
}

Result<NoneType, std::string> AudioRecorder::start() {
  if (!isIdle()) {
    return Err("Recorder is already recording");
  }

  // Taken before the control mutex: it can wait seconds for a Bluetooth link, and every other
  // control path would wait behind it.
  auto route = input_->acquireRoute();

  Result<NoneType, std::string> startResult = Ok(None);
  RecorderOutputs::Detached abandoned;

  {
    std::scoped_lock controlLock(controlMutex_);

    if (!isIdle()) {
      return Err("Recorder is already recording");
    }

    auto openResult = input_->open(
        std::move(route),
        InputOpenPurpose::StartSession,
        inputGate_,
        RecorderInputEvents(weak_from_this(), session_.inputGeneration));
    if (openResult.is_err()) {
      return openResult;
    }

    startResult = input_->start();

    if (startResult.is_ok()) {
      auto formatResult = input_->getCurrentFormat();
      if (formatResult.is_ok()) {
        session_.format = formatResult.unwrap();
        startResult = outputs_.prepare(*session_.format);
      } else {
        startResult = Err("Recorder input format is unavailable: " + formatResult.unwrap_err());
      }
    }

    if (startResult.is_ok()) {
      enterPhase(Phase::Recording);
      return Ok(None);
    }

    abandoned = endSession();
  }

  RecorderOutputs::finalize(std::move(abandoned));
  return startResult;
}

Result<FileInfo, std::string> AudioRecorder::stop() {
  RecorderOutputs::Detached detached;

  {
    std::scoped_lock controlLock(controlMutex_);

    if (isIdle()) {
      return Err("Recorder is not in recording state.");
    }

    detached = endSession();
  }

  return RecorderOutputs::finalize(std::move(detached));
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
  const auto phaseBefore = session_.phase.load(std::memory_order_acquire);
  enterPhase(Phase::Paused);

  auto resumeResult = input_->resume();
  auto formatResult = input_->getCurrentFormat();

  if (formatResult.is_ok()) {
    // Also after a refused resume: the call may still have reconfigured the input (iOS rebuilds
    // its graph before it tries to start it), and capture can come back later with no
    // notification.
    if (auto repointResult = repointOutputsAt(formatResult.unwrap()); repointResult.is_err()) {
      reportError(repointResult.unwrap_err());
    }
  } else if (resumeResult.is_ok()) {
    // A running input whose format cannot be read would leave the session reading as recording
    // while nothing reaches the outputs, so the resume is undone instead.
    input_->pause();
  }

  const bool isCapturing = resumeResult.is_ok() && formatResult.is_ok();
  enterPhase(isCapturing ? Phase::Recording : phaseBefore);

  if (resumeResult.is_err()) {
    return resumeResult;
  }
  if (formatResult.is_err()) {
    return Err("Recorder input format is unavailable after resuming: " + formatResult.unwrap_err());
  }
  return Ok(None);
}

void AudioRecorder::pause() {
  std::scoped_lock controlLock(controlMutex_);

  switch (session_.phase.load(std::memory_order_acquire)) {
    case Phase::Recording:
      // Not when the system already stopped the input: resume() restarts that one.
      if (input_->isRunning()) {
        enterPhase(Phase::Paused);
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

Result<NoneType, std::string> AudioRecorder::rerouteInput(
    std::optional<uint64_t> openedGeneration) {
  auto reopenResult = reopenInput(openedGeneration);

  if (reopenResult.is_err()) {
    // Through the handle, which keeps the file info for consumeLastRecordingResult(), so what
    // was recorded so far is not lost. The handle only stops the session it started.
    if (ActiveRecorderHandle::global().stopAndReturnInfo(shared_from_this()).is_err() &&
        !isIdle()) {
      stop();
    }
    reportError("Recorder input could not be reopened: " + reopenResult.unwrap_err());
  }
  return reopenResult;
}

void AudioRecorder::onInputFormatChanged(uint64_t openedGeneration) {
  std::scoped_lock controlLock(controlMutex_);

  if (isIdle() || openedGeneration != session_.inputGeneration) {
    return;
  }

  // Unreadable while the platform rebuilds its input graph. The gate is left as it is: arming it
  // would feed frames of an unknown format to the outputs, and disarming it would silence the
  // session for good if no later notification arrives.
  auto formatResult = input_->getCurrentFormat();
  if (formatResult.is_err()) {
    return;
  }

  if (auto repointResult = repointOutputsAt(formatResult.unwrap()); repointResult.is_err()) {
    reportError(repointResult.unwrap_err());
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

Result<NoneType, std::string> AudioRecorder::enableFileOutput(
    std::shared_ptr<AudioFileProperties> properties) {
  std::scoped_lock controlLock(controlMutex_);

  if (!isIdle()) {
    return Err("File output cannot be changed while a recording session is active");
  }

  return outputs_.enableFile(std::move(properties));
}

void AudioRecorder::disableFileOutput() {
  RecorderOutputs::Detached detached;

  {
    std::scoped_lock controlLock(controlMutex_);
    detached = outputs_.disableFile();
  }

  RecorderOutputs::finalize(std::move(detached));
}

void AudioRecorder::connect(const std::shared_ptr<utils::graph::NodeHandle> &node) {
  std::scoped_lock controlLock(controlMutex_);
  outputs_.connect(node, session_.format);
}

void AudioRecorder::disconnect() {
  RecorderOutputs::Detached detached;

  {
    std::scoped_lock controlLock(controlMutex_);
    detached = outputs_.disconnect();
  }

  RecorderOutputs::finalize(std::move(detached));
}

Result<NoneType, std::string> AudioRecorder::setOnAudioReadyCallback(
    float sampleRate,
    size_t bufferLength,
    int channelCount,
    uint64_t callbackId) {
  std::scoped_lock controlLock(controlMutex_);
  return outputs_.setCallback(sampleRate, bufferLength, channelCount, callbackId, session_.format);
}

void AudioRecorder::clearOnAudioReadyCallback() {
  std::scoped_lock controlLock(controlMutex_);
  outputs_.clearCallback();
}

void AudioRecorder::setOnErrorCallback(uint64_t callbackId) {
  std::scoped_lock controlLock(controlMutex_);
  outputs_.setErrorCallbackId(callbackId);
}

void AudioRecorder::clearOnErrorCallback() {
  std::scoped_lock controlLock(controlMutex_);
  outputs_.setErrorCallbackId(0);
}

double AudioRecorder::getCurrentDuration() const {
  return session_.recordedSeconds.load(std::memory_order_relaxed);
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

double AudioRecorder::getInputLatency() const {
  if (isIdle()) {
    return 0.0;
  }

  return input_->latency(session_.lastBufferSeconds.load(std::memory_order_relaxed));
}

void AudioRecorder::onInputFrames(const float *const *channels, int channelCount, int numFrames) {
  if (channels == nullptr || numFrames <= 0 ||
      channelCount != session_.format->layout.channelCount) {
    return;
  }

  const double bufferSeconds = static_cast<double>(numFrames) / session_.format->layout.sampleRate;
  session_.lastBufferSeconds.store(bufferSeconds, std::memory_order_relaxed);
  // The only writer while the gate is armed, so a load and a store need no read-modify-write.
  session_.recordedSeconds.store(
      session_.recordedSeconds.load(std::memory_order_relaxed) + bufferSeconds,
      std::memory_order_relaxed);

  outputs_.write(channels, numFrames);
}

void AudioRecorder::enterPhase(Phase phase) {
  if (phase != Phase::Recording) {
    inputGate_->disarmAndWait();
  }

  session_.phase.store(phase, std::memory_order_release);

  if (phase == Phase::Recording) {
    inputGate_->arm();
  }
}

Result<NoneType, std::string> AudioRecorder::repointOutputsAt(const StreamFormat &format) {
  if (format == *session_.format) {
    return Ok(None);
  }

  // Nothing may deliver frames in the old format once the outputs start following the new one.
  inputGate_->disarmAndWait();
  session_.format = format;
  auto followResult = outputs_.follow(format);

  if (session_.phase.load(std::memory_order_acquire) == Phase::Recording) {
    inputGate_->arm();
  }
  return followResult;
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
      enterPhase(Phase::Rerouting);
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

  auto openResult = input_->open(
      std::move(route),
      InputOpenPurpose::ContinueSession,
      inputGate_,
      RecorderInputEvents(weak_from_this(), closedGeneration));
  if (openResult.is_err()) {
    return openResult;
  }

  const Phase phaseToRestore = session_.phaseAfterReroute.load(std::memory_order_acquire);
  if (phaseToRestore == Phase::Recording) {
    auto startResult = input_->start();
    if (startResult.is_err()) {
      return startResult;
    }
  }

  // Read only now: starting can reconfigure the input.
  auto formatResult = input_->getCurrentFormat();
  if (formatResult.is_err()) {
    return Err("Recorder input format is unavailable: " + formatResult.unwrap_err());
  }

  auto outputsResult = repointOutputsAt(formatResult.unwrap());
  if (outputsResult.is_err()) {
    return outputsResult;
  }

  enterPhase(phaseToRestore);
  return Ok(None);
}

RecorderOutputs::Detached AudioRecorder::endSession() {
  enterPhase(Phase::Idle);
  input_->close();
  ++session_.inputGeneration;
  session_.format.reset();
  session_.lastBufferSeconds.store(0.0, std::memory_order_relaxed);
  session_.recordedSeconds.store(0.0, std::memory_order_relaxed);

  return outputs_.detach();
}

void AudioRecorder::reportError(const std::string &message) {
  const uint64_t callbackId = outputs_.errorCallbackId();

  if (audioEventHandlerRegistry_ == nullptr || callbackId == 0) {
    return;
  }

  audioEventHandlerRegistry_->dispatchEvent(
      AudioEvent::RECORDER_ERROR, callbackId, StringPayload{.name = "message", .reason = message});
}

} // namespace audioapi
