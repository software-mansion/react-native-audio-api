#include <audioapi/android/core/AndroidAudioRecorder.h>
#include <audioapi/android/core/utils/AndroidFileWriterBackend.h>
#include <audioapi/android/core/utils/AndroidRecorderCallback.h>

#if !RN_AUDIO_API_FFMPEG_DISABLED
#include <audioapi/android/core/utils/ffmpegBackend/FFmpegFileWriter.h>
#endif // RN_AUDIO_API_FFMPEG_DISABLED

#include <audioapi/android/core/utils/miniaudioBackend/MiniAudioFileWriter.h>
#include <audioapi/core/sources/RecorderAdapterNode.h>
#include <audioapi/core/utils/Constants.h>
#include <audioapi/core/utils/Locker.h>
#include <audioapi/events/AudioEventHandlerRegistry.h>
#include <audioapi/utils/AudioArray.h>
#include <audioapi/utils/AudioBus.h>
#include <audioapi/utils/AudioFileProperties.h>
#include <audioapi/utils/CircularAudioArray.h>
#include <audioapi/utils/CircularOverflowableAudioArray.h>

#include <memory>
#include <string>

namespace audioapi {

AndroidAudioRecorder::AndroidAudioRecorder(
    const std::shared_ptr<AudioEventHandlerRegistry> &audioEventHandlerRegistry)
    : AudioRecorder(audioEventHandlerRegistry),
      streamSampleRate_(0.0),
      streamChannelCount_(0),
      streamMaxBufferSizeInFrames_(0) {}

/// @brief Destructor ensures that the audio stream and each output type are closed and flushed up remaining data.
/// TODO: Possibly locks here are not necessary, but we might have an issue with oboe having raw pointer to the
/// recorder (and player) instances, thus creating race conditions during destruction.
/// callable from the JS thread only (i hope).
AndroidAudioRecorder::~AndroidAudioRecorder() {
  {
    std::scoped_lock dtorLock(callbackMutex_, fileWriterMutex_, adapterNodeMutex_);

    if (usesFileOutput()) {
      fileOutputEnabled_.store(false, std::memory_order_release);
      fileWriter_->closeFile();
    }

    if (usesCallback()) {
      callbackOutputEnabled_.store(false, std::memory_order_release);
      dataCallback_->cleanup();
    }

    if (isConnected()) {
      isConnected_.store(false, std::memory_order_release);
      adapterNode_->cleanup();
    }
  }

  if (mStream_) {
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
  if (mStream_) {
    return Result<NoneType, std::string>::Ok(None);
  }

  oboe::AudioStreamBuilder builder;
  builder.setSharingMode(oboe::SharingMode::Exclusive)
      ->setDirection(oboe::Direction::Input)
      ->setFormat(oboe::AudioFormat::Float)
      ->setFormatConversionAllowed(true)
      ->setPerformanceMode(oboe::PerformanceMode::None)
      ->setSampleRateConversionQuality(oboe::SampleRateConversionQuality::Medium)
      ->setDataCallback(this)
      ->setErrorCallback(this);

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
Result<std::string, std::string> AndroidAudioRecorder::start(const std::string &fileNameOverride) {
  std::scoped_lock startLock(callbackMutex_, fileWriterMutex_, adapterNodeMutex_);

  if (!isIdle()) {
    return Result<std::string, std::string>::Err("Recorder is already recording");
  }

  auto streamResult = openAudioStream();

  if (!streamResult.is_ok()) {
    return Result<std::string, std::string>::Err(streamResult.unwrap_err());
  }

  if (!mStream_) {
    return Result<std::string, std::string>::Err("Audio stream is not initialized.");
  }

  if (usesFileOutput()) {
    auto fileResult = std::static_pointer_cast<AndroidFileWriterBackend>(fileWriter_)
                          ->openFile(
                              streamSampleRate_,
                              streamChannelCount_,
                              streamMaxBufferSizeInFrames_,
                              fileNameOverride);

    if (!fileResult.is_ok()) {
      return Result<std::string, std::string>::Err(
          "Failed to open file for writing: " + fileResult.unwrap_err());
    }

    filePath_ = fileResult.unwrap();
  }

  if (usesCallback()) {
    std::static_pointer_cast<AndroidRecorderCallback>(dataCallback_)
        ->prepare(streamSampleRate_, streamChannelCount_, streamMaxBufferSizeInFrames_);
  }

  if (isConnected()) {
    deinterleavingBuffer_ = std::make_shared<AudioArray>(streamMaxBufferSizeInFrames_);
    adapterNode_->init(streamMaxBufferSizeInFrames_, streamChannelCount_);
  }

  auto result = mStream_->requestStart();

  if (result != oboe::Result::OK) {
    return Result<std::string, std::string>::Err(
        "Failed to start stream: " + std::string(oboe::convertToText(result)));
  }

  state_.store(RecorderState::Recording, std::memory_order_release);
  return Result<std::string, std::string>::Ok(std::format("file://{}", filePath_));
}

/// @brief Stops the audio stream and finalizes any output (file writing, callback, adapter node).
/// This method should be called from the JS thread only.
/// @returns On success, returns the file URI, size in MB and duration in seconds of the recorded file (if file output is enabled).
/// NOTE: due to the file access nature on Android, the size might sometimes be zeroed (really long files).
Result<std::tuple<std::string, double, double>, std::string> AndroidAudioRecorder::stop() {
  std::scoped_lock stopLock(callbackMutex_, fileWriterMutex_, adapterNodeMutex_);

  std::string filePath = std::format("file://{}", filePath_);
  double outputFileSize = 0.0;
  double outputDuration = 0.0;

  if (isIdle()) {
    return Result<std::tuple<std::string, double, double>, std::string>::Err(
        "Recorder is not in recording state.");
  }

  if (!mStream_) {
    return Result<std::tuple<std::string, double, double>, std::string>::Err(
        "Audio stream is not initialized.");
  }

  state_.store(RecorderState::Idle, std::memory_order_release);
  mStream_->requestStop();

  if (usesFileOutput()) {
    auto fileResult = fileWriter_->closeFile();

    if (!fileResult.is_ok()) {
      return Result<std::tuple<std::string, double, double>, std::string>::Err(
          "Failed to close file: " + fileResult.unwrap_err());
    }

    outputFileSize = std::get<0>(fileResult.unwrap());
    outputDuration = std::get<1>(fileResult.unwrap());
  }

  if (usesCallback()) {
    dataCallback_->cleanup();
  }

  if (isConnected()) {
    adapterNode_->cleanup();
  }

  filePath_ = "";
  return Result<std::tuple<std::string, double, double>, std::string>::Ok(
      {filePath, outputFileSize, outputDuration});
}

/// @brief Enables file output for the recorder with the specified properties.
/// If the recorder is already active, it will prepare and open the file for writing immediately.
/// Due to the nature of RN this might be called multiple times during recording session (especially during development),
/// thus the requirement of handling the "already active" case.
/// This method should be called from the JS thread only.
/// @param properties Properties defining the audio file format and encoding options.
/// @returns On success, returns the file URI where the recording is being saved, otherwise returns an error message.
Result<std::string, std::string> AndroidAudioRecorder::enableFileOutput(
    std::shared_ptr<AudioFileProperties> properties) {
  std::scoped_lock fileWriterLock(fileWriterMutex_);

  if (properties->format == AudioFileProperties::Format::WAV) {
    fileWriter_ = std::make_shared<MiniAudioFileWriter>(audioEventHandlerRegistry_, properties);
  } else {
#if !RN_AUDIO_API_FFMPEG_DISABLED
    fileWriter_ = std::make_shared<android::ffmpeg::FFmpegAudioFileWriter>(
        audioEventHandlerRegistry_, properties);
#else
    return Result<std::string, std::string>::Err(
        "FFmpeg backend is disabled. Cannot create file writer for the requested format. Use WAV format instead.");
#endif
  }

  if (!isIdle()) {
    auto fileResult =
        std::static_pointer_cast<AndroidFileWriterBackend>(fileWriter_)
            ->openFile(streamSampleRate_, streamChannelCount_, streamMaxBufferSizeInFrames_, "");

    if (!fileResult.is_ok()) {
      return Result<std::string, std::string>::Err(
          "Failed to open file for writing: " + fileResult.unwrap_err());
    }

    filePath_ = fileResult.unwrap();
  }

  fileOutputEnabled_.store(true, std::memory_order_release);
  return Result<std::string, std::string>::Ok(filePath_);
}

/// @brief Disables file output for the recorder.
/// If the recorder is currently active, it will finalize and close the file immediately.
/// This method should be called from the JS thread only.
void AndroidAudioRecorder::disableFileOutput() {
  std::scoped_lock fileWriterLock(fileWriterMutex_);
  fileOutputEnabled_.store(false, std::memory_order_release);
  fileWriter_ = nullptr;
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
  std::scoped_lock callbackLock(callbackMutex_);
  dataCallback_ = std::make_shared<AndroidRecorderCallback>(
      audioEventHandlerRegistry_, sampleRate, bufferLength, channelCount, callbackId);

  if (!isIdle()) {
    std::static_pointer_cast<AndroidRecorderCallback>(dataCallback_)
        ->prepare(streamSampleRate_, streamChannelCount_, streamMaxBufferSizeInFrames_);
  }

  callbackOutputEnabled_.store(true, std::memory_order_release);

  return Result<NoneType, std::string>::Ok(None);
}

/// @brief Clears the audio data callback.
/// If the recorder is currently active, it will stop invoking the callback immediately.
/// This method should be called from the JS thread only.
void AndroidAudioRecorder::clearOnAudioReadyCallback() {
  std::scoped_lock callbackLock(callbackMutex_);
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

  if (!isIdle()) {
    deinterleavingBuffer_ = std::make_shared<AudioArray>(streamMaxBufferSizeInFrames_);
    adapterNode_->init(streamMaxBufferSizeInFrames_, streamChannelCount_);
  }

  isConnected_.store(true, std::memory_order_release);
}

/// @brief Disconnects the currently connected RecorderAdapterNode from the recorder.
/// If the recorder is currently active, it will stop routing audio data immediately.
/// This method should be called from the JS thread only.
void AndroidAudioRecorder::disconnect() {
  std::scoped_lock adapterLock(adapterNodeMutex_);
  isConnected_.store(false, std::memory_order_release);
  deinterleavingBuffer_ = nullptr;
  adapterNode_ = nullptr;
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
      std::static_pointer_cast<AndroidFileWriterBackend>(fileWriter_)
          ->writeAudioData(audioData, numFrames);
    }
  }

  if (usesCallback()) {
    if (auto callbackLock = Locker::tryLock(callbackMutex_)) {
      std::static_pointer_cast<AndroidRecorderCallback>(dataCallback_)
          ->receiveAudioData(audioData, numFrames);
    }
  }

  if (isConnected()) {
    if (auto adapterLock = Locker::tryLock(adapterNodeMutex_)) {
      for (int channel = 0; channel < streamChannelCount_; ++channel) {
        for (int frame = 0; frame < numFrames; ++frame) {
          deinterleavingBuffer_->getData()[frame] =
              static_cast<float *>(audioData)[frame * streamChannelCount_ + channel];
        }

        adapterNode_->buff_[channel]->write(deinterleavingBuffer_->getData(), numFrames);
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

  if (mStream_) {
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
      // TODO: call error callback
      return;
    }

    mStream_->requestStart();
    state_.store(RecorderState::Recording, std::memory_order_release);
  }
}

} // namespace audioapi
