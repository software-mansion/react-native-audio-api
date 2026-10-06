#include <android/log.h>
#include <audioapi/android/core/AndroidAudioRecorder.h>
#include <audioapi/android/system/NativeInputRouting.h>

#include <audioapi/core/inputs/ActiveRecorderHandle.h>
#include <audioapi/core/utils/AudioFileWriter.h>
#include <audioapi/core/utils/AudioRecorderCallback.h>
#include <audioapi/core/utils/Constants.h>

#include <audioapi/core/utils/Locker.h>
#include <audioapi/events/IAudioEventHandlerRegistry.h>
#include <audioapi/utils/AudioFileProperties.h>
#include <audioapi/utils/AudioRecorderOptions.h>
#include <audioapi/utils/CircularArray.hpp>
#include <audioapi/utils/CircularOverflowableAudioArray.h>
#include <algorithm>

#include <memory>
#include <optional>
#include <string>
#include <utility>

namespace audioapi {

namespace {
/// Oboe's preset for each option. PlatformDefault sets none, which keeps Oboe's own default.
std::optional<oboe::InputPreset> toOboeInputPreset(
    AudioRecorderOptions::AndroidInputPreset preset) {
  using AndroidInputPreset = AudioRecorderOptions::AndroidInputPreset;
  switch (preset) {
    case AndroidInputPreset::PlatformDefault:
      return std::nullopt;
    case AndroidInputPreset::Generic:
      return oboe::InputPreset::Generic;
    case AndroidInputPreset::Camcorder:
      return oboe::InputPreset::Camcorder;
    case AndroidInputPreset::VoiceRecognition:
      return oboe::InputPreset::VoiceRecognition;
    case AndroidInputPreset::VoiceCommunication:
      return oboe::InputPreset::VoiceCommunication;
    case AndroidInputPreset::Unprocessed:
      return oboe::InputPreset::Unprocessed;
    case AndroidInputPreset::VoicePerformance:
      return oboe::InputPreset::VoicePerformance;
  }
  return std::nullopt;
}
} // namespace

AndroidAudioRecorder::AndroidAudioRecorder(
    const std::shared_ptr<IAudioEventHandlerRegistry> &audioEventHandlerRegistry,
    AudioRecorderOptions options)
    : AudioRecorder(audioEventHandlerRegistry),
      inputPreset_(options.androidInputPreset),
      streamDeviceId_(NativeInputRouting::kSystemDefaultDeviceId),
      captureDeviceId_(NativeInputRouting::kSystemDefaultDeviceId) {}

/// @brief Destructor ensures that the audio stream and each output type are closed and flushed up remaining data.
/// callable from the JS thread or handled by audio thread (if js dropped recorder first).
AndroidAudioRecorder::~AndroidAudioRecorder() {
  stop();

  // stop() leaves an idle recorder alone, but a start() that failed after configuring some of
  // its side effects leaves those behind on one.
  DetachedSideEffects leftovers;
  {
    std::scoped_lock lock(callbackMutex_, fileWriterMutex_, adapterNodeMutex_);
    leftovers = detachSideEffects();
    callbackOutputState_.store(OutputState::Disabled, std::memory_order_release);
    fileOutputState_.store(OutputState::Disabled, std::memory_order_release);
    connectionState_.store(OutputState::Disabled, std::memory_order_release);
    dataCallback_ = nullptr;
  }
  finalizeSideEffects(std::move(leftovers));

  // Closing the stream waits for the callback in flight, so nothing is destroyed under it.
  cleanup();
}

/// @brief Creates and opens the Oboe audio input stream for recording.
/// calculates the "native" or hardware stream parameters for other interfaces
/// to use.
/// Called from start() on the promise thread pool and from a restart on the
/// Oboe error thread or the thread MediaSessionManager reroutes on.
/// @returns Success status or Error status with message.
Result<NoneType, std::string> AndroidAudioRecorder::openAudioStream() {
  const int32_t preferredDeviceId = captureDeviceId_;

  if (mStream_ != nullptr) {
    if (streamDeviceId_ == preferredDeviceId) {
      return Ok(None);
    }

    closeStream();
  }

  oboe::AudioStreamBuilder builder;
  builder.setSharingMode(oboe::SharingMode::Exclusive)
      ->setDirection(oboe::Direction::Input)
      ->setFormat(oboe::AudioFormat::Float)
      ->setFormatConversionAllowed(true)
      ->setPerformanceMode(oboe::PerformanceMode::None)
      ->setSampleRateConversionQuality(oboe::SampleRateConversionQuality::Medium)
      ->setDataCallback(shared_from_this())
      ->setErrorCallback(shared_from_this());

  if (auto preset = toOboeInputPreset(inputPreset_)) {
    builder.setInputPreset(*preset);
  }

  if (preferredDeviceId != NativeInputRouting::kSystemDefaultDeviceId) {
    builder.setDeviceId(preferredDeviceId);
  }

  auto result = builder.openStream(mStream_);

  if (result != oboe::Result::OK || mStream_ == nullptr) {
    return Err("Failed to open audio stream: " + std::string(oboe::convertToText(result)));
  }

  // The selection is a preference, as on iOS: when the platform routes elsewhere
  // (the device is gone, or OpenSL ES, which ignores setDeviceId and reports
  // kUnspecified), recording continues on the routed device instead of failing.
  if (preferredDeviceId != NativeInputRouting::kSystemDefaultDeviceId &&
      mStream_->getDeviceId() != preferredDeviceId) {
    __android_log_print(
        ANDROID_LOG_WARN,
        "AndroidAudioRecorder",
        "Input device %d was requested, but the capture stream opened on device %d",
        preferredDeviceId,
        mStream_->getDeviceId());
  }

  streamDeviceId_ = preferredDeviceId;
  streamSampleRate_.store(static_cast<float>(mStream_->getSampleRate()), std::memory_order_release);
  streamChannelCount_ = mStream_->getChannelCount();
  streamMaxBufferSizeInFrames_ = mStream_->getBufferSizeInFrames();

  if (streamChannelCount_ > MAX_CHANNEL_COUNT) {
    mStream_->close();
    mStream_ = nullptr;
    return Err("Input channel count exceeds MAX_CHANNEL_COUNT");
  }
  planarInput_ = AudioBuffer(
      static_cast<size_t>(std::max(streamMaxBufferSizeInFrames_, 1)),
      streamChannelCount_,
      streamSampleRate_.load(std::memory_order_acquire));

  return Ok(None);
}

/// @brief prepares and starts the audio recording process.
/// If audio stream is opened correctly, it will set up any output configured
/// (file writing, callback, adapter node) and start the stream.
/// This method should be called from the JS thread only.
/// NOTE: I've noticed some possibly invalid file paths being returned on Android,
/// RN side requires their "file://" prefix, but sometimes it returned raw path.
/// Most likely this was due to alpha version mistakes, but in case of problems leaving this here. (ㆆ _ ㆆ)
/// @returns On success, returns the file URI where the recording is being saved (if file output is enabled).
Result<NoneType, std::string> AndroidAudioRecorder::start() {
  if (!isIdle()) {
    return Err("Recorder is already recording");
  }

  // Acquired before taking the recorder's mutexes: it can wait seconds for a Bluetooth link,
  // and getters called from the JS thread must not be stuck behind that wait.
  const auto routedDeviceId = NativeInputRouting::acquireInputRoute();

  std::scoped_lock startLock(callbackMutex_, fileWriterMutex_, adapterNodeMutex_, streamMutex_);

  if (!isIdle()) {
    NativeInputRouting::releaseInputRoute();
    return Err("Recorder is already recording");
  }

  adoptInputRoute(routedDeviceId);

  Result<NoneType, std::string> result =
      Err("The selected Bluetooth microphone could not be connected.");
  if (routedDeviceId.has_value()) {
    result = startCapture(std::nullopt, RecorderState::Recording);
  }

  if (!result.is_ok()) {
    releaseInputRoute();
  }
  return result;
}

Result<NoneType, std::string> AndroidAudioRecorder::startCapture(
    const std::optional<StreamFormat> &preparedFormat,
    RecorderState targetState) {
  auto streamResult = openAudioStream();

  if (!streamResult.is_ok()) {
    return streamResult;
  }

  auto formatResult = resolveStreamFormat();

  if (!formatResult.is_ok()) {
    return Err("Audio stream is not initialized.");
  }

  if (preparedFormat != formatResult.unwrap()) {
    auto outputsResult = prepareOutputs(formatResult.unwrap());

    if (!outputsResult.is_ok()) {
      return outputsResult;
    }
  }

  if (targetState == RecorderState::Recording) {
    auto result = mStream_->requestStart();

    if (result != oboe::Result::OK) {
      return Err("Failed to start stream: " + std::string(oboe::convertToText(result)));
    }
  }

  state_.store(targetState, std::memory_order_release);
  return Ok(None);
}

void AndroidAudioRecorder::adoptInputRoute(std::optional<int32_t> routedDeviceId) {
  holdsInputRoute_ = true;
  captureDeviceId_ = routedDeviceId.value_or(NativeInputRouting::kSystemDefaultDeviceId);
}

void AndroidAudioRecorder::releaseInputRoute() {
  if (!holdsInputRoute_) {
    return;
  }

  holdsInputRoute_ = false;
  NativeInputRouting::releaseInputRoute();
}

/// @brief Stops the audio stream and finalizes any output (file writing, callback, adapter node).
/// This method should be called from the JS thread only.
/// @returns On success, returns the file URI, size in MB and duration in seconds of the recorded file (if file output is enabled).
/// NOTE: due to the file access nature on Android, the size might sometimes be zeroed (really long files).
Result<FileInfo, std::string> AndroidAudioRecorder::stop() {
  DetachedSideEffects sideEffects;

  {
    std::scoped_lock stopLock(callbackMutex_, fileWriterMutex_, adapterNodeMutex_, streamMutex_);

    if (isIdle()) {
      return Err("Recorder is not in recording state.");
    }

    state_.store(RecorderState::Idle, std::memory_order_release);
    releaseInputRoute();
    lastCallbackFrameCount_.store(0, std::memory_order_release);

    // There is no stream while restartCapture() is between closing the old one and opening
    // the new one; the outputs are finalized all the same.
    if (mStream_ != nullptr) {
      mStream_->requestStop();
    }

    sideEffects = detachSideEffects();
  }

  return finalizeSideEffects(std::move(sideEffects));
}

/// @brief Pauses the audio recording stream.
/// For session without active file output, this method acts same as stop().
/// This method should be called from the JS thread only.
void AndroidAudioRecorder::pause() {
  std::scoped_lock streamLock(streamMutex_);
  if (!isStreamRecording()) {
    return;
  }

  mStream_->pause(0);
  state_.store(RecorderState::Paused, std::memory_order_release);
}

/// @brief Resumes the audio recording stream if it was previously paused.
/// This method should be called from the JS thread only.
void AndroidAudioRecorder::resume() {
  std::scoped_lock streamLock(streamMutex_);
  // No stream while a restart is under way; the restart brings the session back paused.
  if (!isPaused() || mStream_ == nullptr) {
    return;
  }

  mStream_->start(0);
  state_.store(RecorderState::Recording, std::memory_order_release);
}

/// @brief onAudioReady callback that is invoked by the Oboe stream when new audio data is available.
/// This method runs on the audio thread.
/// It routes the audio data to the enabled side effects: file writer, callback, and adapter node.
/// For safety measures (check note about RN of enableFileOutput), each side effect is protected by a lock
/// additionally to the enabled checks.
/// @param oboeStream Pointer to the Oboe audio stream.
/// @param audioData Pointer to the audio data buffer (interleaved float samples).
/// @param numFrames Number of audio frames in the data buffer.
/// @returns DataCallbackResult indicating whether to continue or stop the stream.
oboe::DataCallbackResult AndroidAudioRecorder::onAudioReady(
    oboe::AudioStream *oboeStream,
    void *audioData,
    int32_t numFrames) {
  if (isPaused()) {
    return oboe::DataCallbackResult::Continue;
  }

  const auto frames = static_cast<size_t>(numFrames);
  if (audioData == nullptr || numFrames <= 0 || frames > planarInput_.getSize()) {
    return oboe::DataCallbackResult::Continue;
  }

  planarInput_.deinterleaveFrom(static_cast<const float *>(audioData), frames);
  const float *channels[MAX_CHANNEL_COUNT];
  for (int channel = 0; channel < streamChannelCount_; ++channel) {
    channels[channel] = planarInput_.getChannel(channel)->begin();
  }
  onAudioFrames(channels, numFrames);

  return oboe::DataCallbackResult::Continue;
}

Result<StreamFormat, std::string> AndroidAudioRecorder::resolveStreamFormat() const {
  const auto sampleRate = streamSampleRate_.load(std::memory_order_acquire);

  if (sampleRate <= 0.0F || streamChannelCount_ <= 0 || streamMaxBufferSizeInFrames_ <= 0) {
    return Err("audio stream is not initialized");
  }

  return Ok(
      StreamFormat{
          .layout = {.sampleRate = sampleRate, .channelCount = streamChannelCount_},
          .maxFramesPerBuffer = static_cast<size_t>(streamMaxBufferSizeInFrames_)});
}

bool AndroidAudioRecorder::isRecording() const {
  std::scoped_lock streamLock(streamMutex_);
  return isStreamRecording();
}

bool AndroidAudioRecorder::isStreamRecording() const {
  return mStream_ != nullptr &&
      state_.load(std::memory_order_acquire) == RecorderState::Recording &&
      mStream_->getState() == oboe::StreamState::Started;
}

bool AndroidAudioRecorder::isPaused() const {
  return state_.load(std::memory_order_acquire) == RecorderState::Paused;
}

bool AndroidAudioRecorder::isIdle() const {
  return state_.load(std::memory_order_acquire) == RecorderState::Idle;
}

void AndroidAudioRecorder::cleanup() {
  std::scoped_lock streamLock(streamMutex_);
  state_.store(RecorderState::Idle, std::memory_order_release);
  releaseInputRoute();
  closeStream();
}

void AndroidAudioRecorder::closeStream() {
  if (mStream_ != nullptr) {
    mStream_->requestStop();
    mStream_->close();
    mStream_.reset();
  }
}

Result<NoneType, std::string> AndroidAudioRecorder::rerouteInput() {
  return restartCapture();
}

Result<NoneType, std::string> AndroidAudioRecorder::restartCapture() {
  std::optional<StreamFormat> previousFormat;
  RecorderState stateToRestore = RecorderState::Idle;

  {
    std::scoped_lock streamLock(streamMutex_);

    // No stream on a non-idle recorder means another restart is already under way.
    if (isIdle() || mStream_ == nullptr) {
      return Ok(None);
    }

    auto formatResult = resolveStreamFormat();
    if (formatResult.is_ok()) {
      previousFormat = formatResult.unwrap();
    }
    stateToRestore = state_.load(std::memory_order_acquire);
    closeStream();
    releaseInputRoute();
  }

  // Outside the recorder's mutexes for the same reason as in start(). stop() may run in
  // this window; it finds no stream and finalizes the session.
  const auto routedDeviceId = NativeInputRouting::acquireInputRoute();
  if (!routedDeviceId.has_value()) {
    __android_log_print(
        ANDROID_LOG_WARN,
        "AndroidAudioRecorder",
        "The selected input could not be routed, continuing on the default input");
  }

  Result<NoneType, std::string> result = Ok(None);
  {
    std::scoped_lock restartLock(callbackMutex_, fileWriterMutex_, adapterNodeMutex_, streamMutex_);

    // Stopped, or stopped and started again, while the route was being acquired.
    if (isIdle() || mStream_ != nullptr) {
      NativeInputRouting::releaseInputRoute();
      return Ok(None);
    }

    adoptInputRoute(routedDeviceId);
    result = startCapture(previousFormat, stateToRestore);
  }

  if (!result.is_ok()) {
    // Stopping through the handle keeps the file info for AudioRecorder.consumeLastRecordingResult(),
    // so what was recorded so far is not lost. The handle only stops the session it started.
    if (ActiveRecorderHandle::global().stopAndReturnInfo(shared_from_this()).is_err() &&
        !isIdle()) {
      stop();
    }
    cleanup();
    reportError("Android recorder error: " + result.unwrap_err());
  }
  return result;
}

/// @brief onError callback that is invoked by the Oboe stream when an error occurs.
/// This method runs on a background thread spawned by Oboe as per AudioStreamAAudio::internalErrorCallback.
/// If the error is a disconnection, it attempts to reopen the stream and resume recording.
/// @param oboeStream Pointer to the Oboe audio stream.
/// @param error The oboe::Result error code.
void AndroidAudioRecorder::onErrorAfterClose(oboe::AudioStream *stream, oboe::Result error) {
  if (error != oboe::Result::ErrorDisconnected) {
    return;
  }

  {
    std::scoped_lock streamLock(streamMutex_);

    // Since this runs on a background thread, it can be delayed, so do not teardown an already healthy stream.
    if (mStream_.get() != stream) {
      return;
    }

    if (isIdle()) {
      releaseInputRoute();
      closeStream();
      return;
    }
  }

  restartCapture();
}

double AndroidAudioRecorder::getInputLatency() const {
  std::scoped_lock streamLock(streamMutex_);

  const auto sampleRate = streamSampleRate_.load(std::memory_order_acquire);

  if (mStream_ == nullptr || isIdle() || sampleRate <= 0.0F) {
    return 0.0;
  }

  const auto latencyResult = mStream_->calculateLatencyMillis();
  if (latencyResult && latencyResult.value() > 0.0) {
    return latencyResult.value() / 1000.0;
  }

  const int32_t callbackFrames = lastCallbackFrameCount_.load(std::memory_order_acquire);
  if (callbackFrames > 0) {
    return static_cast<double>(callbackFrames) / static_cast<double>(sampleRate);
  }

  const int32_t framesPerBurst = mStream_->getFramesPerBurst();
  if (framesPerBurst > 0) {
    return static_cast<double>(framesPerBurst) / static_cast<double>(sampleRate);
  }

  return 0.0;
}

} // namespace audioapi
