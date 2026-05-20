#include <android/log.h>
#include <audioapi/android/core/AndroidAudioRecorder.h>
#include <audioapi/android/core/utils/AndroidFileWriterBackend.h>
#include <audioapi/android/core/utils/AndroidRecorderCallback.h>

#if !RN_AUDIO_API_FFMPEG_DISABLED
#include <audioapi/android/core/utils/ffmpegBackend/FFmpegFileWriter.h>
#endif // RN_AUDIO_API_FFMPEG_DISABLED

#include <audioapi/android/core/utils/AndroidRotatingFileWriter.h>
#include <audioapi/android/core/utils/miniaudioBackend/MiniAudioFileWriter.h>
#include <audioapi/core/sources/RecorderAdapterNode.h>
#include <audioapi/core/utils/Constants.h>
#include <audioapi/core/utils/Locker.h>
#include <audioapi/events/AudioEventHandlerRegistry.h>
#include <audioapi/utils/AudioFileProperties.h>
#include <audioapi/utils/CircularArray.hpp>
#include <audioapi/utils/CircularOverflowableAudioArray.h>

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace audioapi {

AndroidAudioRecorder::AndroidAudioRecorder(
    const std::shared_ptr<AudioEventHandlerRegistry> &audioEventHandlerRegistry)
    : AudioRecorder(audioEventHandlerRegistry),
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
  if (adapterNode_ != nullptr) {
    adapterNode_->adapterCleanup();
  }
  // oboe could be handling stopping and closing the stream, sanity check just in case
  if (mStream_ != nullptr) {
    mStream_->requestStop();
    mStream_->close();
    mStream_.reset();
  }
}

/// @brief Creates and opens the Oboe audio input stream for recording.
/// calculates the "native" or hardware stream parameters for other interfaces
/// to use.
/// Callable from the JS thread only.
/// @returns Success status or Error status with message.
Result<NoneType, std::string> AndroidAudioRecorder::openAudioStream() {
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
  std::scoped_lock startLock(callbackMutex_, fileWriterMutex_, adapterNodeMutex_);

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
    std::static_pointer_cast<AndroidRecorderCallback>(dataCallback_)
        ->prepare(streamSampleRate_, streamChannelCount_, streamMaxBufferSizeInFrames_);
    callbackOutputConfigured_.store(true, std::memory_order_release);
  }

  if (wantsConnection() && adapterNode_ != nullptr) {
    deinterleavingBuffer_ = std::make_shared<AudioBuffer>(
        streamMaxBufferSizeInFrames_, streamChannelCount_, streamSampleRate_);
    adapterNode_->init(streamMaxBufferSizeInFrames_, streamChannelCount_, streamSampleRate_);
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
  std::shared_ptr<RecorderAdapterNode> adapterNode;
  std::vector<std::string> outputPaths;

  double outputFileSize = 0.0;
  double outputDuration = 0.0;
  bool hadFileOutput = false;

  {
    std::scoped_lock stopLock(callbackMutex_, fileWriterMutex_, adapterNodeMutex_);

    if (isIdle()) {
      return Result<std::tuple<std::vector<std::string>, double, double>, std::string>::Err(
          "Recorder is not in recording state.");
    }

    if (mStream_ == nullptr) {
      return Result<std::tuple<std::vector<std::string>, double, double>, std::string>::Err(
          "Audio stream is not initialized.");
    }

    state_.store(RecorderState::Idle, std::memory_order_release);
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
      adapterNode = std::move(adapterNode_);
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

  if (adapterNode != nullptr) {
    adapterNode->adapterCleanup();
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
  if (!isRecording()) {
    return;
  }

  mStream_->pause(0);
  state_.store(RecorderState::Paused, std::memory_order_release);
}

/// @brief Resumes the audio recording stream if it was previously paused.
/// This method should be called from the JS thread only.
void AndroidAudioRecorder::resume() {
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
    std::static_pointer_cast<AndroidRecorderCallback>(dataCallback_)
        ->prepare(streamSampleRate_, streamChannelCount_, streamMaxBufferSizeInFrames_);
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
void AndroidAudioRecorder::connect(const std::shared_ptr<RecorderAdapterNode> &node) {
  std::scoped_lock adapterLock(adapterNodeMutex_);
  adapterNode_ = node;
  isConnected_.store(true, std::memory_order_release);
  connectedConfigured_.store(false, std::memory_order_release);

  if (!isIdle()) {
    deinterleavingBuffer_ = std::make_shared<AudioBuffer>(
        streamMaxBufferSizeInFrames_, streamChannelCount_, streamSampleRate_);
    adapterNode_->init(streamMaxBufferSizeInFrames_, streamChannelCount_, streamSampleRate_);
    connectedConfigured_.store(true, std::memory_order_release);
  }
}

/// @brief Disconnects the currently connected RecorderAdapterNode from the recorder.
/// If the recorder is currently active, it will stop routing audio data immediately.
/// This method should be called from the JS thread only.
void AndroidAudioRecorder::disconnect() {
  std::shared_ptr<RecorderAdapterNode> adapterNode;
  bool hadConnection = false;
  {
    std::scoped_lock adapterLock(adapterNodeMutex_);
    hadConnection = isConnected();
    connectedConfigured_.store(false, std::memory_order_release);
    isConnected_.store(false, std::memory_order_release);
    deinterleavingBuffer_ = nullptr;
    adapterNode = std::move(adapterNode_);
  }

  if (hadConnection && adapterNode != nullptr) {
    adapterNode->adapterCleanup();
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
      auto adapterNode = adapterNode_;
      auto deinterleavingBuffer = deinterleavingBuffer_;
      if (!isConnected() || adapterNode == nullptr || deinterleavingBuffer == nullptr) {
        return oboe::DataCallbackResult::Continue;
      }

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
  return state_.load(std::memory_order_acquire) == RecorderState::Recording &&
      mStream_->getState() == oboe::StreamState::Started;
}

bool AndroidAudioRecorder::isPaused() const {
  return state_.load(std::memory_order_acquire) == RecorderState::Paused;
}

bool AndroidAudioRecorder::isIdle() const {
  return state_.load(std::memory_order_acquire) == RecorderState::Idle;
}

void AndroidAudioRecorder::cleanup() {
  state_.store(RecorderState::Idle, std::memory_order_release);

  if (mStream_ != nullptr) {
    mStream_->close();
    mStream_.reset();
  }
}

/// @brief onError callback that is invoked by the Oboe stream when an error occurs.
/// This method runs on the audio thread.
/// If the error is a disconnection, it attempts to reopen the stream and resume recording.
/// @param oboeStream Pointer to the Oboe audio stream.
/// @param error The oboe::Result error code.
void AndroidAudioRecorder::onErrorAfterClose(oboe::AudioStream *stream, oboe::Result error) {
  if (error == oboe::Result::ErrorDisconnected) {
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

} // namespace audioapi
