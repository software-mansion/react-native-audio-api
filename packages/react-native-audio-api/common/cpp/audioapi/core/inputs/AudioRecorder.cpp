#include <audioapi/core/inputs/AudioRecorder.h>

#include <audioapi/core/inputs/IRecorderInput.h>
#include <audioapi/core/inputs/RecorderInputEvents.h>
#include <audioapi/core/inputs/RecorderInputGate.h>
#include <audioapi/core/inputs/RecordingSessionRegistry.h>
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
    std::shared_ptr<RecordingSessionRegistry> sessionRegistry,
    const PlatformFileBackend *fileBackend)
    : input_(std::move(input)),
      inputGate_(
          std::make_shared<RecorderInputGate>(
              [this](const float *const *channels, int channelCount, int numFrames) {
                onInputFrames(channels, channelCount, numFrames);
              })),
      outputs_(audioEventHandlerRegistry, fileBackend != nullptr ? *fileBackend : osFileBackend()),
      audioEventHandlerRegistry_(audioEventHandlerRegistry),
      sessionRegistry_(std::move(sessionRegistry)) {}

/// The platform input can outlive the recorder and keeps calling the gate, whose sink holds a raw
/// pointer to it: disarm before any teardown, whatever state the recorder is in.
AudioRecorder::~AudioRecorder() {
  inputGate_->disarmAndWait();
  stop();
}

Result<NoneType, std::string> AudioRecorder::start() {
  uint64_t startingSession = 0;
  uint64_t routedGeneration = 0;

  {
    std::scoped_lock controlLock(controlMutex_);

    if (!isIdle()) {
      return Err("Recorder is already recording");
    }

    auto claimResult = sessionRegistry_->claim(shared_from_this());
    if (claimResult.is_err()) {
      return Err(claimResult.unwrap_err());
    }
    startingSession = session_.id = claimResult.unwrap();
    routedGeneration = session_.inputGeneration;
    setSessionState({.phase = Phase::Starting});
  }

  Result<NoneType, std::string> startResult = Ok(None);
  RecorderOutputs::Detached abandoned;

  while (true) {
    // With the control mutex released: it can wait seconds for a Bluetooth link, and every other
    // control path would wait behind it.
    auto route = input_->acquireRoute();

    std::scoped_lock controlLock(controlMutex_);

    // Stopped, or stopped and started again, while the route was on its way.
    const auto startingState = getSessionState();
    if (startingState.phase != Phase::Starting || session_.id != startingSession) {
      return Err("Recorder was stopped before it started");
    }

    // A reroute arrived meanwhile: this route was settled before the device changed.
    if (session_.inputGeneration != routedGeneration) {
      routedGeneration = session_.inputGeneration;
      continue;
    }

    startResult = input_->open(
        std::move(route),
        InputOpenPurpose::StartSession,
        inputGate_,
        RecorderInputEvents(weak_from_this(), session_.inputGeneration));

    // Paused while the route was on its way: the input opens stopped, as after a reroute.
    if (startResult.is_ok() && !startingState.isPaused) {
      startResult = input_->start();
    }

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
      setSessionState({.phase = Phase::Open, .isPaused = startingState.isPaused});
      return Ok(None);
    }

    abandoned = endSession();
    break;
  }

  RecorderOutputs::finalize(std::move(abandoned));
  return startResult;
}

Result<FileInfo, std::string> AudioRecorder::stop(std::optional<uint64_t> openedGeneration) {
  uint64_t stoppedSession = 0;
  RecorderOutputs::Detached detached;

  {
    std::scoped_lock controlLock(controlMutex_);

    if (isIdle()) {
      return Err("Recorder is not in recording state.");
    }
    if (openedGeneration.has_value() && *openedGeneration != session_.inputGeneration) {
      return Err("Recorder input was reopened since.");
    }

    stoppedSession = session_.id;
    detached = endSession();
  }

  auto stopResult = RecorderOutputs::finalize(std::move(detached));
  if (stopResult.is_ok()) {
    sessionRegistry_->saveRecording(stoppedSession, stopResult.unwrap());
  }
  return stopResult;
}

Result<NoneType, std::string> AudioRecorder::resume() {
  std::scoped_lock controlLock(controlMutex_);

  const auto stateBefore = getSessionState();
  switch (stateBefore.phase) {
    case Phase::Idle:
      return Err("Recorder is not in recording state.");
    case Phase::Starting:
    case Phase::Rerouting:
      // The input opens started once its route is in.
      setSessionState({.phase = stateBefore.phase, .isPaused = false});
      return Ok(None);
    case Phase::Open:
      if (!stateBefore.isPaused && input_->isRunning()) {
        return Ok(None);
      }
      // Paused, or the system stopped the input under a recording session.
      break;
  }

  // The input may come back in another format, so nothing is delivered until the outputs
  // follow it.
  setSessionState({.phase = Phase::Open, .isPaused = true});

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
  setSessionState(isCapturing ? SessionState{.phase = Phase::Open} : stateBefore);

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

  const auto stateBefore = getSessionState();
  if (stateBefore.isPaused) {
    return;
  }

  switch (stateBefore.phase) {
    case Phase::Idle:
      return;
    case Phase::Starting:
    case Phase::Rerouting:
      setSessionState({.phase = stateBefore.phase, .isPaused = true});
      return;
    case Phase::Open:
      setSessionState({.phase = Phase::Open, .isPaused = true});
      input_->pause();
      return;
  }
}

Result<NoneType, std::string> AudioRecorder::rerouteInput(
    std::optional<uint64_t> openedGeneration) {
  uint64_t failedGeneration = 0;
  auto reopenResult = reopenInput(openedGeneration, failedGeneration);

  if (reopenResult.is_err()) {
    // Bound to the failed opening: the control mutex was released on the way here, and a later
    // reroute may have reopened the input since.
    stop(failedGeneration);
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
  return getSessionState().phase == Phase::Idle;
}

bool AudioRecorder::isRecording() const {
  return getState() == RecorderState::Recording;
}

bool AudioRecorder::isPaused() const {
  return getState() == RecorderState::Paused;
}

RecorderState AudioRecorder::getState() const {
  const auto current = getSessionState();
  if (current.phase == Phase::Idle) {
    return RecorderState::Idle;
  }
  if (current.isPaused) {
    return RecorderState::Paused;
  }
  // An open input the system stopped (an interrupted iOS session) reads as paused.
  if (current.phase == Phase::Open && !input_->isRunning()) {
    return RecorderState::Paused;
  }
  return RecorderState::Recording;
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

AudioRecorder::SessionState AudioRecorder::getSessionState() const {
  return session_.state.load(std::memory_order_acquire);
}

void AudioRecorder::setSessionState(SessionState next) {
  if (!next.feedsOutputs()) {
    inputGate_->disarmAndWait();
  }

  session_.state.store(next, std::memory_order_release);

  if (next.feedsOutputs()) {
    inputGate_->arm();
  }
}

Result<NoneType, std::string> AudioRecorder::repointOutputsAt(const StreamFormat &format) {
  if (format == *session_.format) {
    return Ok(None);
  }

  // Nothing may deliver frames in the old format once the outputs are reprepared for the new one.
  inputGate_->disarmAndWait();
  session_.format = format;
  auto reprepareResult = outputs_.reprepare(format);

  if (getSessionState().feedsOutputs()) {
    inputGate_->arm();
  }
  return reprepareResult;
}

Result<NoneType, std::string> AudioRecorder::reopenInput(
    std::optional<uint64_t> openedGeneration,
    uint64_t &reopenedGeneration) {
  uint64_t closedGeneration = 0;

  {
    std::scoped_lock controlLock(controlMutex_);

    const auto current = getSessionState();
    const bool isStaleOpening =
        openedGeneration.has_value() && *openedGeneration != session_.inputGeneration;
    if (current.phase == Phase::Idle || isStaleOpening) {
      return Ok(None);
    }
    // No input is open yet: the start acquires its route again once this one is in.
    if (current.phase == Phase::Starting) {
      ++session_.inputGeneration;
      return Ok(None);
    }

    // A reroute that arrives while another waits for its route takes over from it.
    if (current.phase == Phase::Open) {
      setSessionState({.phase = Phase::Rerouting, .isPaused = current.isPaused});
    }

    input_->close();
    closedGeneration = ++session_.inputGeneration;
  }
  reopenedGeneration = closedGeneration;

  // With the control mutex released, for the same reason as in start().
  auto route = input_->acquireRoute();

  std::scoped_lock controlLock(controlMutex_);

  // Stopped, or taken over by a later reroute, while the route was on its way.
  const auto reroutingState = getSessionState();
  if (reroutingState.phase != Phase::Rerouting || session_.inputGeneration != closedGeneration) {
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

  if (!reroutingState.isPaused) {
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

  setSessionState({.phase = Phase::Open, .isPaused = reroutingState.isPaused});
  return Ok(None);
}

RecorderOutputs::Detached AudioRecorder::endSession() {
  setSessionState({.phase = Phase::Idle});
  input_->close();
  ++session_.inputGeneration;
  session_.format.reset();
  session_.lastBufferSeconds.store(0.0, std::memory_order_relaxed);
  session_.recordedSeconds.store(0.0, std::memory_order_relaxed);
  sessionRegistry_->release(weak_from_this());

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
