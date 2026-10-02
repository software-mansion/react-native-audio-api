#include <android/log.h>
#include <audioapi/android/core/AndroidAudioRecorder.h>
#include <audioapi/android/core/utils/AndroidFileWriterBackend.h>
#include <audioapi/android/core/utils/AndroidRecorderCallback.h>
#include <audioapi/android/system/NativeInputRouting.h>

#if !RN_AUDIO_API_FFMPEG_DISABLED
#include <audioapi/android/core/utils/ffmpegBackend/FFmpegFileWriter.h>
#endif // RN_AUDIO_API_FFMPEG_DISABLED

#include <audioapi/android/core/utils/AndroidRotatingFileWriter.h>
#include <audioapi/android/core/utils/miniaudioBackend/MiniAudioFileWriter.h>
#include <audioapi/core/inputs/ActiveRecorderHandle.h>
#include <audioapi/core/sources/RecorderAdapterNode.h>
#include <audioapi/core/utils/Constants.h>
#include <audioapi/core/utils/Locker.h>
#include <audioapi/events/IAudioEventHandlerRegistry.h>
#include <audioapi/utils/AudioFileProperties.h>
#include <audioapi/utils/AudioRecorderOptions.h>
#include <audioapi/utils/CircularArray.hpp>
#include <audioapi/utils/CircularOverflowableAudioArray.h>

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace audioapi {

namespace {
/// Maps the JS-facing preset name to Oboe's InputPreset. An unknown or empty
/// name yields no preset call, preserving Oboe's own default
/// (InputPreset::VoiceRecognition) exactly as before this option existed.
std::optional<oboe::InputPreset> inputPresetFromString(const std::string &name) {
  if (name == "generic") {
    return oboe::InputPreset::Generic;
  }
  if (name == "camcorder") {
    return oboe::InputPreset::Camcorder;
  }
  if (name == "voiceRecognition") {
    return oboe::InputPreset::VoiceRecognition;
  }
  if (name == "voiceCommunication") {
    return oboe::InputPreset::VoiceCommunication;
  }
  if (name == "unprocessed") {
    return oboe::InputPreset::Unprocessed;
  }
  if (name == "voicePerformance") {
    return oboe::InputPreset::VoicePerformance;
  }
  return std::nullopt;
}

/// Runs an action when the scope ends unless dismiss() is called first. Lets a
/// multi-step operation roll back an early step on every failure path without
/// repeating the rollback before each return.
RecorderAdapterNode *adapterNodeOf(const std::shared_ptr<utils::graph::NodeHandle> &handle) {
  return static_cast<RecorderAdapterNode *>(handle->audioNode.get());
}

template <typename Action>
class ScopeExit {
 public:
  explicit ScopeExit(Action action) : action_(std::move(action)) {}
  ~ScopeExit() {
    if (armed_) {
      action_();
    }
  }

  DELETE_COPY_AND_MOVE(ScopeExit);

  void dismiss() {
    armed_ = false;
  }

 private:
  Action action_;
  bool armed_ = true;
};
} // namespace

AndroidAudioRecorder::AndroidAudioRecorder(
    const std::shared_ptr<IAudioEventHandlerRegistry> &audioEventHandlerRegistry,
    AudioRecorderOptions options)
    : AudioRecorder(audioEventHandlerRegistry),
      inputPreset_(std::move(options.androidInputPreset)),
      streamSampleRate_(0.0),
      streamChannelCount_(0),
      streamMaxBufferSizeInFrames_(0),
      streamDeviceId_(NativeInputRouting::kSystemDefaultDeviceId) {}

/// @brief Destructor ensures that the audio stream and each output type are closed and flushed up remaining data.
/// callable from the JS thread or handled by audio thread (if js dropped recorder first).
AndroidAudioRecorder::~AndroidAudioRecorder() {
  // there is no need to lock here, as there could be two threads that can destruct js gc and audio thread one (or one created by it)
  // if we are on js:
  // audio thread dropped recorder so onAudioReady callback would not be called anymore
  //
  // if we are on audio thread:
  // js dropped recorder and oboe states that "callback object cannot be deleted before the stream is deleted"
  if (fileWriter_ != nullptr) {
    fileWriter_->closeFile();
  }
  if (dataCallback_ != nullptr) {
    dataCallback_->cleanup();
  }
  if (adapterNodeHandle_ != nullptr) {
    adapterNodeOf(adapterNodeHandle_)->adapterCleanup();
  }

  cleanup();
}

/// @brief Creates and opens the Oboe audio input stream for recording.
/// calculates the "native" or hardware stream parameters for other interfaces
/// to use.
/// Called from start() on the promise thread pool and from onErrorAfterClose()
/// on the Oboe error thread.
/// The caller must hold an input route (see adoptInputRoute). An already open
/// stream bound to a device other than that route's is closed and reopened.
/// @returns Success status or Error status with message.
Result<NoneType, std::string> AndroidAudioRecorder::openAudioStream() {
  std::scoped_lock streamLock(streamMutex_);

  const int32_t preferredDeviceId = captureDeviceId_;

  if (mStream_ != nullptr) {
    if (streamDeviceId_ == preferredDeviceId) {
      return Result<NoneType, std::string>::Ok(None);
    }

    closeAudioStream();
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

  if (auto preset = inputPresetFromString(inputPreset_)) {
    builder.setInputPreset(*preset);
  }

  if (preferredDeviceId != NativeInputRouting::kSystemDefaultDeviceId) {
    builder.setDeviceId(preferredDeviceId);
  }

  auto result = builder.openStream(mStream_);

  if (result != oboe::Result::OK || mStream_ == nullptr) {
    return Result<NoneType, std::string>::Err(
        "Failed to open audio stream: " + std::string(oboe::convertToText(result)));
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
  streamSampleRate_ = static_cast<float>(mStream_->getSampleRate());
  streamChannelCount_ = mStream_->getChannelCount();
  streamMaxBufferSizeInFrames_ = mStream_->getBufferSizeInFrames();

  return Result<NoneType, std::string>::Ok(None);
}

/// @brief prepares and starts the audio recording process.
/// If audio stream is opened correctly, it will set up any output configured
/// (file writing, callback, adapter node) and start the stream.
/// This method should be called from the JS thread only.
/// NOTE: I've noticed some possibly invalid file paths being returned on Android,
/// RN side requires their "file://" prefix, but sometimes it returned raw path.
/// Most likely this was due to alpha version mistakes, but in case of problems leaving this here. (ㆆ _ ㆆ)
/// @returns On success, returns the file URI where the recording is being saved (if file output is enabled).
Result<NoneType, std::string> AndroidAudioRecorder::start(const std::string &fileNameOverride) {
  if (!isIdle()) {
    return Result<NoneType, std::string>::Err("Recorder is already recording");
  }

  // Acquired before taking the recorder's mutexes: it can wait seconds for a Bluetooth link,
  // and getters called from the JS thread must not be stuck behind that wait.
  const auto routedDeviceId = NativeInputRouting::acquireInputRoute();

  std::scoped_lock startLock(callbackMutex_, fileWriterMutex_, adapterNodeMutex_, streamMutex_);

  if (!isIdle()) {
    NativeInputRouting::releaseInputRoute();
    return Result<NoneType, std::string>::Err("Recorder is already recording");
  }

  adoptInputRoute(routedDeviceId);
  ScopeExit releaseRoute([this] { releaseInputRoute(); });

  if (!routedDeviceId.has_value()) {
    return Result<NoneType, std::string>::Err(
        "The selected Bluetooth microphone could not be connected.");
  }

  auto streamResult = openAudioStream();

  if (!streamResult.is_ok()) {
    return Result<NoneType, std::string>::Err(streamResult.unwrap_err());
  }

  if (mStream_ == nullptr) {
    return Result<NoneType, std::string>::Err("Audio stream is not initialized.");
  }

  if (wantsFileOutput()) {
    recordingSegmentPaths_.clear();
    closedSegmentsSizeMb_ = 0.0;
    closedSegmentsDuration_ = 0.0;
    auto writerResult = setupFileWriter(fileProperties_, fileNameOverride);
    if (!writerResult.is_ok()) {
      return writerResult;
    }
    __android_log_print(
        ANDROID_LOG_INFO,
        "AndroidAudioRecorder",
        "File created successfully at path: %s",
        filePath_.c_str());
  }

  if (wantsCallback()) {
    if (dataCallback_ == nullptr) {
      return Result<NoneType, std::string>::Err("Callback output is unavailable.");
    }

    dataCallback_->setOnErrorCallback(errorCallbackId_.load(std::memory_order_acquire));
    auto callbackResult = prepareCallback();
    if (!callbackResult.is_ok()) {
      return callbackResult;
    }
  }

  if (wantsConnection() && adapterNodeHandle_ != nullptr) {
    prepareAdapterNode();
  }

  auto result = mStream_->requestStart();

  if (result != oboe::Result::OK) {
    return Result<NoneType, std::string>::Err(
        "Failed to start stream: " + std::string(oboe::convertToText(result)));
  }

  releaseRoute.dismiss();
  state_.store(RecorderState::Recording, std::memory_order_release);
  return Result<NoneType, std::string>::Ok(None);
}

void AndroidAudioRecorder::adoptInputRoute(std::optional<int32_t> routedDeviceId) {
  std::scoped_lock streamLock(streamMutex_);
  holdsInputRoute_ = true;
  captureDeviceId_ = routedDeviceId.value_or(NativeInputRouting::kSystemDefaultDeviceId);
}

void AndroidAudioRecorder::releaseInputRoute() {
  std::scoped_lock streamLock(streamMutex_);

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
Result<std::tuple<std::vector<std::string>, double, double>, std::string>
AndroidAudioRecorder::stop() {
  std::shared_ptr<AudioFileWriter> fileWriter;
  std::shared_ptr<AudioRecorderCallback> dataCallback;
  std::shared_ptr<utils::graph::NodeHandle> adapterNodeHandle;
  std::vector<std::string> outputPaths;

  double outputFileSize = 0.0;
  double outputDuration = 0.0;
  double closedSegmentsSizeMb = 0.0;
  double closedSegmentsDuration = 0.0;
  bool hadFileOutput = false;

  {
    std::scoped_lock stopLock(callbackMutex_, fileWriterMutex_, adapterNodeMutex_, streamMutex_);

    if (isIdle()) {
      return Result<std::tuple<std::vector<std::string>, double, double>, std::string>::Err(
          "Recorder is not in recording state.");
    }

    state_.store(RecorderState::Idle, std::memory_order_release);
    releaseInputRoute();
    lastCallbackFrameCount_.store(0, std::memory_order_release);

    // There is no stream while restartCapture() is between closing the old one and opening
    // the new one; the outputs are finalized all the same.
    if (mStream_ != nullptr) {
      mStream_->requestStop();
    }

    hadFileOutput = usesFileOutput();

    if (hadFileOutput) {
      fileOutputConfigured_.store(false, std::memory_order_release);
      fileWriter = std::move(fileWriter_);
      closedSegmentsSizeMb = closedSegmentsSizeMb_;
      closedSegmentsDuration = closedSegmentsDuration_;
    }

    if (usesCallback()) {
      callbackOutputConfigured_.store(false, std::memory_order_release);
      dataCallback = std::move(dataCallback_);
    }

    if (isConnected()) {
      connectedConfigured_.store(false, std::memory_order_release);
      adapterNodeHandle = std::move(adapterNodeHandle_);
    }
  }

  for (const auto &raw : recordingSegmentPaths_) {
    if (!raw.empty()) {
      outputPaths.push_back(std::format("file://{}", raw));
    }
  }
  if (hadFileOutput && outputPaths.empty() && !filePath_.empty()) {
    outputPaths.push_back(std::format("file://{}", filePath_));
  }

  recordingSegmentPaths_.clear();
  filePath_ = "";

  if (fileWriter != nullptr) {
    auto fileResult = fileWriter->closeFile();

    if (!fileResult.is_ok()) {
      return Result<std::tuple<std::vector<std::string>, double, double>, std::string>::Err(
          "Failed to close file: " + fileResult.unwrap_err());
    }

    outputFileSize = closedSegmentsSizeMb + std::get<0>(fileResult.unwrap());
    outputDuration = closedSegmentsDuration + std::get<1>(fileResult.unwrap());
  }

  if (dataCallback != nullptr) {
    dataCallback->cleanup();
  }

  if (adapterNodeHandle != nullptr) {
    adapterNodeOf(adapterNodeHandle)->adapterCleanup();
  }

  return Result<std::tuple<std::vector<std::string>, double, double>, std::string>::Ok(
      std::make_tuple(std::move(outputPaths), outputFileSize, outputDuration));
}

/// @brief Enables file output for the recorder with the specified properties.
/// The file itself is created by the next start(). An active (recording or paused) session keeps
/// the output it started with, so calling this during a session fails and changes nothing.
/// This method should be called from the JS thread only.
/// @param properties Properties defining the audio file format and encoding options.
/// @returns Ok when the properties were stored, otherwise an error message.
Result<NoneType, std::string> AndroidAudioRecorder::enableFileOutput(
    std::shared_ptr<AudioFileProperties> properties) {
  std::scoped_lock lock(fileWriterMutex_, streamMutex_);

  if (!isIdle()) {
    return Result<NoneType, std::string>::Err(
        "File output cannot be changed while a recording session is active");
  }

  fileProperties_ = properties;
  fileOutputEnabled_.store(true, std::memory_order_release);
  fileOutputConfigured_.store(false, std::memory_order_release);

  return Result<NoneType, std::string>::Ok(None);
}

std::shared_ptr<AudioFileWriter> AndroidAudioRecorder::createFileWriter(
    const std::shared_ptr<AudioFileProperties> &props) {
  if (props->format == AudioFileProperties::Format::WAV) {
    return std::make_shared<MiniAudioFileWriter>(
        audioEventHandlerRegistry_,
        props,
        streamSampleRate_,
        streamChannelCount_,
        streamMaxBufferSizeInFrames_);
  }
#if !RN_AUDIO_API_FFMPEG_DISABLED
  return std::make_shared<android::ffmpeg::FFmpegAudioFileWriter>(
      audioEventHandlerRegistry_,
      props,
      streamSampleRate_,
      streamChannelCount_,
      streamMaxBufferSizeInFrames_);
#else
  return nullptr;
#endif
}

Result<NoneType, std::string> AndroidAudioRecorder::setupFileWriter(
    const std::shared_ptr<AudioFileProperties> &properties,
    const std::string &fileNameOverride) {
#if RN_AUDIO_API_FFMPEG_DISABLED
  if (properties->format != AudioFileProperties::Format::WAV) {
    return Result<NoneType, std::string>::Err(
        "FFmpeg backend is disabled. Cannot create file writer for the requested format. Use WAV format instead.");
  }
#endif

  if (properties->rotateIntervalBytes > 0) {
    fileWriter_ = std::make_shared<AndroidRotatingFileWriter>(
        audioEventHandlerRegistry_,
        properties,
        properties->rotateIntervalBytes,
        [this](const std::shared_ptr<AudioFileProperties> &p) { return createFileWriter(p); },
        [this](const std::string &path) {
          if (!path.empty()) {
            recordingSegmentPaths_.push_back(path);
          }
        });
  } else {
    fileWriter_ = createFileWriter(properties);
  }

  fileWriter_->setOnErrorCallback(errorCallbackId_.load(std::memory_order_acquire));

  auto backend = std::static_pointer_cast<AndroidFileWriterBackend>(fileWriter_);
  auto fileResult = backend->openFile(
      streamSampleRate_, streamChannelCount_, streamMaxBufferSizeInFrames_, fileNameOverride);

  if (!fileResult.is_ok()) {
    fileOutputConfigured_.store(false, std::memory_order_release);
    fileWriter_ = nullptr;
    return Result<NoneType, std::string>::Err(
        "Failed to open file for writing: " + fileResult.unwrap_err());
  }

  filePath_ = fileResult.unwrap();
  if (properties->rotateIntervalBytes == 0) {
    recordingSegmentPaths_.push_back(filePath_);
  }
  fileOutputConfigured_.store(true, std::memory_order_release);
  return Result<NoneType, std::string>::Ok(None);
}

/// @brief Disables file output for the recorder.
/// If the recorder is currently active, it will finalize and close the file immediately.
/// This method should be called from the JS thread only.
void AndroidAudioRecorder::disableFileOutput() {
  std::shared_ptr<AudioFileWriter> fileWriter;
  {
    std::scoped_lock fileWriterLock(fileWriterMutex_);
    fileOutputConfigured_.store(false, std::memory_order_release);
    fileOutputEnabled_.store(false, std::memory_order_release);
    fileWriter = std::move(fileWriter_);
  }

  if (fileWriter != nullptr) {
    fileWriter->closeFile();
  }
}

/// @brief Pauses the audio recording stream.
/// For session without active file output, this method acts same as stop().
/// This method should be called from the JS thread only.
void AndroidAudioRecorder::pause() {
  std::scoped_lock streamLock(streamMutex_);
  if (!isRecording()) {
    return;
  }

  mStream_->pause(0);
  state_.store(RecorderState::Paused, std::memory_order_release);
}

/// @brief Resumes the audio recording stream if it was previously paused.
/// This method should be called from the JS thread only.
void AndroidAudioRecorder::resume() {
  std::scoped_lock streamLock(streamMutex_);
  if (!isPaused()) {
    return;
  }

  mStream_->start(0);
  state_.store(RecorderState::Recording, std::memory_order_release);
}

/// @brief Sets the callback to be invoked when audio data is ready.
/// If the recorder is already active, it will prepare the callback for receiving audio data immediately.
/// This method should be called from the JS thread only.
/// @param sampleRate Desired sample rate for the callback audio data.
/// @param bufferLength Desired buffer length in frames for the callback audio data.
/// @param channelCount Number of channels for the callback audio data.
/// @param callbackId Identifier for the JS callback to be invoked.
/// @returns Success status or Error status with message.
Result<NoneType, std::string> AndroidAudioRecorder::setOnAudioReadyCallback(
    float sampleRate,
    size_t bufferLength,
    int channelCount,
    uint64_t callbackId) {
  std::scoped_lock callbackLock(callbackMutex_, errorCallbackMutex_);
  dataCallback_ = std::make_shared<AndroidRecorderCallback>(
      audioEventHandlerRegistry_, sampleRate, bufferLength, channelCount, callbackId);
  dataCallback_->setOnErrorCallback(errorCallbackId_.load(std::memory_order_acquire));
  callbackOutputEnabled_.store(true, std::memory_order_release);
  callbackOutputConfigured_.store(false, std::memory_order_release);

  if (!isIdle()) {
    return prepareCallback();
  }

  return Result<NoneType, std::string>::Ok(None);
}

/// @brief Clears the audio data callback.
/// If the recorder is currently active, it will stop invoking the callback immediately.
/// This method should be called from the JS thread only.
void AndroidAudioRecorder::clearOnAudioReadyCallback() {
  std::scoped_lock callbackLock(callbackMutex_);
  callbackOutputConfigured_.store(false, std::memory_order_release);
  callbackOutputEnabled_.store(false, std::memory_order_release);
  dataCallback_ = nullptr;
}

/// @brief Connects a RecorderAdapterNode to the recorder for audio data routing.
/// If the recorder is already active, it will initialize the adapter node immediately.
/// This method should be called from the JS thread only.
/// @param node Shared pointer to the RecorderAdapterNode to connect.
void AndroidAudioRecorder::connect(const std::shared_ptr<utils::graph::NodeHandle> &node) {
  std::scoped_lock adapterLock(adapterNodeMutex_);
  adapterNodeHandle_ = node;
  isConnected_.store(true, std::memory_order_release);
  connectedConfigured_.store(false, std::memory_order_release);

  if (!isIdle()) {
    prepareAdapterNode();
  }
}

/// @brief Disconnects the currently connected RecorderAdapterNode from the recorder.
/// If the recorder is currently active, it will stop routing audio data immediately.
/// This method should be called from the JS thread only.
void AndroidAudioRecorder::disconnect() {
  std::shared_ptr<utils::graph::NodeHandle> adapterNodeHandle;
  bool hadConnection = false;
  {
    std::scoped_lock adapterLock(adapterNodeMutex_);
    hadConnection = isConnected();
    connectedConfigured_.store(false, std::memory_order_release);
    isConnected_.store(false, std::memory_order_release);
    deinterleavingBuffer_ = nullptr;
    adapterNodeHandle = std::move(adapterNodeHandle_);
  }

  if (hadConnection && adapterNodeHandle != nullptr) {
    adapterNodeOf(adapterNodeHandle)->adapterCleanup();
  }
}

/// @brief onAudioReady callback that is invoked by the Oboe stream when new audio data is available.
/// This method runs on the audio thread.
/// It routes the audio data to the enabled outputs: file writer, callback, and adapter node.
/// For safety measures (check note about RN of enableFileOutput), each output is protected by a lock
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

  if (numFrames > 0) {
    lastCallbackFrameCount_.store(numFrames, std::memory_order_release);
  }

  if (usesFileOutput()) {
    if (auto fileWriterLock = Locker::tryLock(fileWriterMutex_)) {
      auto fileWriter = fileWriter_;
      if (usesFileOutput() && fileWriter != nullptr) {
        fileWriter->writeAudioData(audioData, numFrames);
      }
    }
  }

  if (usesCallback()) {
    if (auto callbackLock = Locker::tryLock(callbackMutex_)) {
      auto dataCallback = std::static_pointer_cast<AndroidRecorderCallback>(dataCallback_);
      if (usesCallback() && dataCallback != nullptr) {
        dataCallback->receiveAudioData(audioData, numFrames);
      }
    }
  }

  if (isConnected()) {
    if (auto adapterLock = Locker::tryLock(adapterNodeMutex_)) {
      auto adapterNodeHandle = adapterNodeHandle_;
      auto deinterleavingBuffer = deinterleavingBuffer_;
      if (!isConnected() || adapterNodeHandle == nullptr || deinterleavingBuffer == nullptr) {
        return oboe::DataCallbackResult::Continue;
      }

      auto *adapterNode = adapterNodeOf(adapterNodeHandle);

      auto const data = static_cast<float *>(audioData);
      deinterleavingBuffer->deinterleaveFrom(data, numFrames);

      for (size_t ch = 0; ch < streamChannelCount_; ++ch) {
        adapterNode->buff_[ch]->write(*deinterleavingBuffer->getChannel(ch), numFrames);
      }
    }
  }

  return oboe::DataCallbackResult::Continue;
}

bool AndroidAudioRecorder::isRecording() const {
  std::scoped_lock streamLock(streamMutex_);
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
  closeAudioStream();
}

void AndroidAudioRecorder::closeAudioStream() {
  std::scoped_lock streamLock(streamMutex_);

  if (mStream_ != nullptr) {
    mStream_->requestStop();
    mStream_->close();
    mStream_.reset();
  }
}

AndroidAudioRecorder::StreamFormat AndroidAudioRecorder::streamFormat() const {
  return {
      .sampleRate = streamSampleRate_.load(std::memory_order_acquire),
      .channelCount = streamChannelCount_,
      .maxBufferSizeInFrames = streamMaxBufferSizeInFrames_,
  };
}

Result<NoneType, std::string> AndroidAudioRecorder::rerouteInput() {
  return restartCapture();
}

Result<NoneType, std::string> AndroidAudioRecorder::restartCapture() {
  StreamFormat previousFormat{};
  RecorderState stateToRestore = RecorderState::Idle;

  {
    std::scoped_lock streamLock(streamMutex_);

    // No stream on a non-idle recorder means another restart is already under way.
    if (isIdle() || mStream_ == nullptr) {
      return Result<NoneType, std::string>::Ok(None);
    }

    previousFormat = streamFormat();
    stateToRestore = state_.load(std::memory_order_acquire);
    closeAudioStream();
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

  auto result = Result<NoneType, std::string>::Ok(None);
  {
    std::scoped_lock restartLock(callbackMutex_, fileWriterMutex_, adapterNodeMutex_, streamMutex_);

    // Stopped, or stopped and started again, while the route was being acquired.
    if (isIdle() || mStream_ != nullptr) {
      NativeInputRouting::releaseInputRoute();
      return Result<NoneType, std::string>::Ok(None);
    }

    adoptInputRoute(routedDeviceId);
    result = reopenAudioStream(previousFormat, stateToRestore);
  }

  if (!result.is_ok()) {
    endSessionAfterFailedRestart(result.unwrap_err());
  }
  return result;
}

Result<NoneType, std::string> AndroidAudioRecorder::reopenAudioStream(
    const StreamFormat &previousFormat,
    RecorderState stateToRestore) {
  auto streamResult = openAudioStream();
  if (!streamResult.is_ok()) {
    return streamResult;
  }

  if (streamFormat() != previousFormat) {
    auto outputsResult = reprepareOutputs();
    if (!outputsResult.is_ok()) {
      return outputsResult;
    }
  }

  if (stateToRestore == RecorderState::Recording) {
    auto startResult = mStream_->requestStart();
    if (startResult != oboe::Result::OK) {
      return Result<NoneType, std::string>::Err(
          "Failed to start stream: " + std::string(oboe::convertToText(startResult)));
    }
  }

  return Result<NoneType, std::string>::Ok(None);
}

void AndroidAudioRecorder::endSessionAfterFailedRestart(const std::string &reason) {
  // Stopping through the handle finalizes the outputs and keeps the file info for
  // AudioRecorder.consumeLastRecordingResult(), so what was recorded so far is not lost.
  auto stopResult = ActiveRecorderHandle::global().stopAndReturnInfo(shared_from_this());

  // The handle only stops the session it started; end any other one directly.
  if (!stopResult.is_ok() && !isIdle()) {
    stop();
  }

  closeAudioStream();
  reportError(reason);
}

Result<NoneType, std::string> AndroidAudioRecorder::reprepareOutputs() {
  if (usesFileOutput() && fileWriter_ != nullptr) {
    // The encoder was configured for the previous stream format, so the audio that follows
    // goes to a new file; stop() returns every segment.
    const std::string segmentFileName = nextSegmentFileName();
    auto closeResult = fileWriter_->closeFile();
    if (closeResult.is_ok()) {
      closedSegmentsSizeMb_ += std::get<0>(closeResult.unwrap());
      closedSegmentsDuration_ += std::get<1>(closeResult.unwrap());
    }

    fileOutputConfigured_.store(false, std::memory_order_release);
    fileWriter_ = nullptr;

    auto writerResult = setupFileWriter(fileProperties_, segmentFileName);
    if (!writerResult.is_ok()) {
      return writerResult;
    }
  }

  if (usesCallback() && dataCallback_ != nullptr) {
    callbackOutputConfigured_.store(false, std::memory_order_release);
    dataCallback_->cleanup();

    auto callbackResult = prepareCallback();
    if (!callbackResult.is_ok()) {
      return callbackResult;
    }
  }

  if (isConnected() && adapterNodeHandle_ != nullptr) {
    connectedConfigured_.store(false, std::memory_order_release);
    adapterNodeOf(adapterNodeHandle_)->adapterCleanup();
    prepareAdapterNode();
  }

  return Result<NoneType, std::string>::Ok(None);
}

Result<NoneType, std::string> AndroidAudioRecorder::prepareCallback() {
  auto result = std::static_pointer_cast<AndroidRecorderCallback>(dataCallback_)
                    ->prepare(streamSampleRate_, streamChannelCount_, streamMaxBufferSizeInFrames_);
  if (result.is_ok()) {
    callbackOutputConfigured_.store(true, std::memory_order_release);
  }
  return result;
}

void AndroidAudioRecorder::prepareAdapterNode() {
  deinterleavingBuffer_ = std::make_shared<AudioBuffer>(
      streamMaxBufferSizeInFrames_, streamChannelCount_, streamSampleRate_);
  adapterNodeOf(adapterNodeHandle_)
      ->init(streamMaxBufferSizeInFrames_, streamChannelCount_, streamSampleRate_);
  connectedConfigured_.store(true, std::memory_order_release);
}

std::string AndroidAudioRecorder::nextSegmentFileName() const {
  if (recordingSegmentPaths_.empty() || fileProperties_ == nullptr ||
      fileProperties_->rotateIntervalBytes > 0) {
    return "";
  }

  const std::string &firstSegmentPath = recordingSegmentPaths_.front();
  const size_t nameStart = firstSegmentPath.find_last_of('/') + 1;
  const size_t extensionStart = firstSegmentPath.find_last_of('.');
  const size_t nameLength = extensionStart != std::string::npos && extensionStart > nameStart
      ? extensionStart - nameStart
      : std::string::npos;

  return std::format(
      "{}_{}", firstSegmentPath.substr(nameStart, nameLength), recordingSegmentPaths_.size() + 1);
}

void AndroidAudioRecorder::reportError(const std::string &message) {
  uint64_t callbackId = errorCallbackId_.load(std::memory_order_acquire);

  if (audioEventHandlerRegistry_ == nullptr || callbackId == 0) {
    return;
  }

  audioEventHandlerRegistry_->dispatchEvent(
      AudioEvent::RECORDER_ERROR,
      callbackId,
      StringPayload{.name = "message", .reason = "Android recorder error: " + message});
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
      cleanup();
      return;
    }
  }

  restartCapture();
}

double AndroidAudioRecorder::getCurrentDuration() const {
  std::scoped_lock lock(fileWriterMutex_);

  if (!usesFileOutput() || fileWriter_ == nullptr) {
    return 0.0;
  }
  return closedSegmentsDuration_ + fileWriter_->getCurrentDuration();
}

double AndroidAudioRecorder::getInputLatency() const {
  std::scoped_lock streamLock(streamMutex_);

  if (mStream_ == nullptr || isIdle() || streamSampleRate_ <= 0.0f) {
    return 0.0;
  }

  const auto latencyResult = mStream_->calculateLatencyMillis();
  if (latencyResult && latencyResult.value() > 0.0) {
    return latencyResult.value() / 1000.0;
  }

  const int32_t callbackFrames = lastCallbackFrameCount_.load(std::memory_order_acquire);
  if (callbackFrames > 0) {
    return static_cast<double>(callbackFrames) / static_cast<double>(streamSampleRate_);
  }

  const int32_t framesPerBurst = mStream_->getFramesPerBurst();
  if (framesPerBurst > 0) {
    return static_cast<double>(framesPerBurst) / static_cast<double>(streamSampleRate_);
  }

  return 0.0;
}

} // namespace audioapi
