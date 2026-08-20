#include <android/log.h>
#include <audioapi/android/core/AndroidAudioRecorder.h>

#include <audioapi/core/sources/RecorderAdapterNode.h>
#include <audioapi/core/utils/AudioRecorderCallback.h>
#include <audioapi/core/utils/Constants.h>
#include <audioapi/core/utils/EncodedAudioFileWriter.h>
#include <audioapi/core/utils/Locker.h>
#include <audioapi/core/utils/RotatingFileWriter.h>
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
} // namespace

AndroidAudioRecorder::AndroidAudioRecorder(
    const std::shared_ptr<IAudioEventHandlerRegistry> &audioEventHandlerRegistry,
    AudioRecorderOptions options)
    : AudioRecorder(audioEventHandlerRegistry),
      inputPreset_(std::move(options.androidInputPreset)),
      streamSampleRate_(0.0),
      streamChannelCount_(0),
      streamMaxBufferSizeInFrames_(0) {}

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
    static_cast<RecorderAdapterNode *>(adapterNodeHandle_->audioNode.get())->adapterCleanup();
  }

  cleanup();
}

/// @brief Creates and opens the Oboe audio input stream for recording.
/// calculates the "native" or hardware stream parameters for other interfaces
/// to use.
/// Callable from the JS thread only.
/// @returns Success status or Error status with message.
Result<NoneType, std::string> AndroidAudioRecorder::openAudioStream() {
  std::scoped_lock streamLock(streamMutex_);
  if (mStream_ != nullptr) {
    return Result<NoneType, std::string>::Ok(None);
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

  auto result = builder.openStream(mStream_);

  if (result != oboe::Result::OK || mStream_ == nullptr) {
    return Result<NoneType, std::string>::Err(
        "Failed to open audio stream: " + std::string(oboe::convertToText(result)));
  }

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
  std::scoped_lock startLock(callbackMutex_, fileWriterMutex_, adapterNodeMutex_, streamMutex_);

  if (!isIdle()) {
    return Result<NoneType, std::string>::Err("Recorder is already recording");
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
    dataCallback_->prepare(streamSampleRate_, streamChannelCount_, streamMaxBufferSizeInFrames_);
    callbackOutputConfigured_.store(true, std::memory_order_release);
  }

  if (wantsConnection() && adapterNodeHandle_ != nullptr) {
    deinterleavingBuffer_ = std::make_shared<AudioBuffer>(
        streamMaxBufferSizeInFrames_, streamChannelCount_, streamSampleRate_);
    static_cast<RecorderAdapterNode *>(adapterNodeHandle_->audioNode.get())
        ->init(streamMaxBufferSizeInFrames_, streamChannelCount_, streamSampleRate_);
    connectedConfigured_.store(true, std::memory_order_release);
  }

  auto result = mStream_->requestStart();

  if (result != oboe::Result::OK) {
    return Result<NoneType, std::string>::Err(
        "Failed to start stream: " + std::string(oboe::convertToText(result)));
  }

  state_.store(RecorderState::Recording, std::memory_order_release);
  return Result<NoneType, std::string>::Ok(None);
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
  bool hadFileOutput = false;

  {
    std::scoped_lock stopLock(callbackMutex_, fileWriterMutex_, adapterNodeMutex_, streamMutex_);

    if (isIdle()) {
      return Result<std::tuple<std::vector<std::string>, double, double>, std::string>::Err(
          "Recorder is not in recording state.");
    }

    if (mStream_ == nullptr) {
      return Result<std::tuple<std::vector<std::string>, double, double>, std::string>::Err(
          "Audio stream is not initialized.");
    }

    state_.store(RecorderState::Idle, std::memory_order_release);
    lastCallbackFrameCount_.store(0, std::memory_order_release);
    mStream_->requestStop();

    hadFileOutput = usesFileOutput();

    if (hadFileOutput) {
      fileOutputConfigured_.store(false, std::memory_order_release);
      fileWriter = std::move(fileWriter_);
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

    outputFileSize = std::get<0>(fileResult.unwrap());
    outputDuration = std::get<1>(fileResult.unwrap());
  }

  if (dataCallback != nullptr) {
    dataCallback->cleanup();
  }

  if (adapterNodeHandle != nullptr) {
    static_cast<RecorderAdapterNode *>(adapterNodeHandle->audioNode.get())->adapterCleanup();
  }

  return Result<std::tuple<std::vector<std::string>, double, double>, std::string>::Ok(
      std::make_tuple(std::move(outputPaths), outputFileSize, outputDuration));
}

/// @brief Enables file output for the recorder with the specified properties.
/// If the recorder is already active, it will prepare and open the file for writing immediately.
/// Due to the nature of RN this might be called multiple times during recording session (especially during development),
/// thus the requirement of handling the "already active" case.
/// This method should be called from the JS thread only.
/// @param properties Properties defining the audio file format and encoding options.
/// @returns On success, returns the file URI where the recording is being saved, otherwise returns an error message.
Result<NoneType, std::string> AndroidAudioRecorder::enableFileOutput(
    std::shared_ptr<AudioFileProperties> properties) {
  std::scoped_lock fileWriterLock(fileWriterMutex_);
  fileProperties_ = properties;
  fileOutputEnabled_.store(true, std::memory_order_release);
  fileOutputConfigured_.store(false, std::memory_order_release);

  if (!isIdle()) {
    auto writerResult = setupFileWriter(properties);
    if (!writerResult.is_ok()) {
      fileOutputEnabled_.store(false, std::memory_order_release);
      return writerResult;
    }
  }

  return Result<NoneType, std::string>::Ok(None);
}

std::shared_ptr<AudioFileWriter> AndroidAudioRecorder::createFileWriter(
    const std::shared_ptr<AudioFileProperties> &props) {
  return std::make_shared<EncodedAudioFileWriter>(audioEventHandlerRegistry_, props);
}

Result<NoneType, std::string> AndroidAudioRecorder::setupFileWriter(
    const std::shared_ptr<AudioFileProperties> &properties,
    const std::string &fileNameOverride) {
  if (properties->rotateIntervalBytes > 0) {
    fileWriter_ = std::make_shared<RotatingFileWriter>(
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

  auto fileResult = fileWriter_->openFile(
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
  dataCallback_ = std::make_shared<AudioRecorderCallback>(
      audioEventHandlerRegistry_, sampleRate, bufferLength, channelCount, callbackId);
  dataCallback_->setOnErrorCallback(errorCallbackId_.load(std::memory_order_acquire));
  callbackOutputEnabled_.store(true, std::memory_order_release);
  callbackOutputConfigured_.store(false, std::memory_order_release);

  if (!isIdle()) {
    dataCallback_->prepare(streamSampleRate_, streamChannelCount_, streamMaxBufferSizeInFrames_);
    callbackOutputConfigured_.store(true, std::memory_order_release);
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
    deinterleavingBuffer_ = std::make_shared<AudioBuffer>(
        streamMaxBufferSizeInFrames_, streamChannelCount_, streamSampleRate_);
    static_cast<RecorderAdapterNode *>(adapterNodeHandle_->audioNode.get())
        ->init(streamMaxBufferSizeInFrames_, streamChannelCount_, streamSampleRate_);
    connectedConfigured_.store(true, std::memory_order_release);
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
    static_cast<RecorderAdapterNode *>(adapterNodeHandle->audioNode.get())->adapterCleanup();
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

  // Oboe already delivers interleaved float32 — the format every consumer expects.
  onAudioFrames(static_cast<const float *>(audioData), numFrames);

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

  if (mStream_ != nullptr) {
    mStream_->requestStop();
    mStream_->close();
    mStream_.reset();
  }
}

/// @brief onError callback that is invoked by the Oboe stream when an error occurs.
/// This method runs on a background thread spawned by Oboe as per AudioStreamAAudio::internalErrorCallback.
/// If the error is a disconnection, it attempts to reopen the stream and resume recording.
/// @param oboeStream Pointer to the Oboe audio stream.
/// @param error The oboe::Result error code.
void AndroidAudioRecorder::onErrorAfterClose(oboe::AudioStream *stream, oboe::Result error) {
  std::scoped_lock streamLock(streamMutex_);
  if (error == oboe::Result::ErrorDisconnected) {

    // Since this runs on a background thread, it can be delayed, so do not teardown an already healthy stream.
    if (mStream_.get() != stream) {
      return;
    }

    cleanup();

    auto streamResult = openAudioStream();

    if (!streamResult.is_ok()) {
      uint64_t callbackId = errorCallbackId_.load(std::memory_order_acquire);

      if (audioEventHandlerRegistry_ == nullptr || callbackId == 0) {
        return;
      }

      std::string message = "Android recorder error: " + streamResult.unwrap_err();
      audioEventHandlerRegistry_->dispatchEvent(
          AudioEvent::RECORDER_ERROR,
          callbackId,
          StringPayload{.name = "message", .reason = std::move(message)});
      return;
    }

    mStream_->requestStart();
    state_.store(RecorderState::Recording, std::memory_order_release);
  }
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
