#include <audioapi/core/inputs/AudioRecorder.h>
#include <audioapi/core/sources/RecorderAdapterNode.h>
#include <audioapi/core/utils/AudioFileWriter.h>
#include <audioapi/core/utils/AudioRecorderCallback.h>
#include <audioapi/core/utils/EncodedAudioFileWriter.h>
#include <audioapi/core/utils/Locker.h>
#include <audioapi/core/utils/RotatingFileWriter.h>
#include <audioapi/utils/AudioFileProperties.h>
#include <audioapi/utils/CircularOverflowableAudioArray.h>

#include <algorithm>
#include <memory>
#include <string>
#include <utility>

// NOLINTBEGIN(cppcoreguidelines-pro-type-static-cast-downcast)

namespace audioapi {

/// @brief Hands one buffer of recorded audio to every configured consumer.
/// Called on the audio thread; must not block, allocate, or throw.
void AudioRecorder::onAudioFrames(const float *interleavedFrames, int numFrames) {
  if (interleavedFrames == nullptr || numFrames <= 0) {
    return;
  }

  lastCallbackFrameCount_.store(numFrames, std::memory_order_release);

  if (usesFileOutput()) {
    auto fileWriterLock = Locker::tryLock(fileWriterMutex_);
    if (fileWriterLock && fileWriter_) {
      // if we are inside the lock and the fileWriter_ is valid we can be sure that nobody will interrupt us in the middle
      fileWriter_->writeAudioData(interleavedFrames, numFrames);
    }
  }

  if (usesCallback()) {
    auto callbackLock = Locker::tryLock(callbackMutex_);
    if (callbackLock && dataCallback_) {
      // if we are inside the lock and the dataCallback_ is valid we can be sure that nobody will interrupt us in the middle
      dataCallback_->receiveAudioData(interleavedFrames, numFrames);
    }
  }

  if (isConnected()) {
    auto adapterLock = Locker::tryLock(adapterNodeMutex_);
    if (!adapterLock || !adapterNodeHandle_ || !deinterleavingBuffer_) {
      return;
    }
    // The buffer is sized for the stream's maximum burst; a larger callback would
    // overrun it, so drop rather than write past the end.
    if (static_cast<size_t>(numFrames) > deinterleavingBuffer_->getSize()) {
      return;
    }

    auto *adapterNode = static_cast<RecorderAdapterNode *>(adapterNodeHandle_->audioNode.get());
    deinterleavingBuffer_->deinterleaveFrom(interleavedFrames, numFrames);

    const size_t channelCount =
        std::min(adapterNode->getChannelCount(), deinterleavingBuffer_->getNumberOfChannels());
    for (size_t channel = 0; channel < channelCount; ++channel) {
      adapterNode->buff_[channel]->write(*deinterleavingBuffer_->getChannel(channel), numFrames);
    }
  }
}

/// @brief Enables file output for the recorder with the specified properties.
/// If the recorder is already active, it opens the file for writing immediately. Due to the
/// nature of RN this might be called multiple times during a recording session (especially
/// during development), thus the requirement of handling the "already active" case.
/// This method should be called from the JS thread only.
/// @param properties Properties defining the audio file format and encoding options.
/// @returns Success status or Error status with message.
Result<NoneType, std::string> AudioRecorder::enableFileOutput(
    std::shared_ptr<AudioFileProperties> properties) {
  std::scoped_lock fileWriterLock(fileWriterMutex_, errorCallbackMutex_);
  fileProperties_ = std::move(properties);
  fileOutputEnabled_.store(true, std::memory_order_release);
  fileOutputConfigured_.store(false, std::memory_order_release);

  if (isIdle()) {
    return Result<NoneType, std::string>::Ok(None);
  }

  auto writerResult = setupFileWriter(fileProperties_);

  if (!writerResult.is_ok()) {
    fileOutputEnabled_.store(false, std::memory_order_release);
    return writerResult;
  }

  return Result<NoneType, std::string>::Ok(None);
}

/// @brief Disables file output for the recorder.
/// If the recorder is currently active, it finalizes and closes the file immediately.
/// This method should be called from the JS thread only.
void AudioRecorder::disableFileOutput() {
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

std::shared_ptr<AudioFileWriter> AudioRecorder::createFileWriter(
    const std::shared_ptr<AudioFileProperties> &properties) {
  return std::make_shared<EncodedAudioFileWriter>(audioEventHandlerRegistry_, properties);
}

/// @brief Opens the output file the recorded frames are written to.
/// A non-zero rotate interval wraps the writer in a RotatingFileWriter, which then builds one
/// writer per segment through the same factory.
/// This method should be called from the JS thread only, with fileWriterMutex_ held.
/// @param properties Properties defining the audio file format and encoding options.
/// @param fileNameOverride Name to write under instead of a generated one, if not empty.
/// @returns Success status or Error status with message.
Result<NoneType, std::string> AudioRecorder::setupFileWriter(
    const std::shared_ptr<AudioFileProperties> &properties,
    const std::string &fileNameOverride) {
  auto formatResult = resolveStreamFormat();

  if (!formatResult.is_ok()) {
    return Result<NoneType, std::string>::Err(
        "Failed to open file for writing: " + formatResult.unwrap_err());
  }

  if (properties->rotateIntervalBytes > 0) {
    fileWriter_ = std::make_shared<RotatingFileWriter>(
        audioEventHandlerRegistry_,
        properties,
        properties->rotateIntervalBytes,
        [this](const std::shared_ptr<AudioFileProperties> &segmentProperties) {
          return createFileWriter(segmentProperties);
        },
        [this](const std::string &path) {
          if (!path.empty()) {
            recordingSegmentPaths_.push_back(path);
          }
        });
  } else {
    fileWriter_ = createFileWriter(properties);
  }

  fileWriter_->setOnErrorCallback(errorCallbackId_.load(std::memory_order_acquire));

  const auto format = formatResult.unwrap();
  auto fileResult = fileWriter_->openFile(
      format.sampleRate, format.channelCount, format.maxFramesPerBuffer, fileNameOverride);

  if (!fileResult.is_ok()) {
    fileOutputConfigured_.store(false, std::memory_order_release);
    fileWriter_ = nullptr;
    return Result<NoneType, std::string>::Err(
        "Failed to open file for writing: " + fileResult.unwrap_err());
  }

  filePath_ = fileResult.unwrap();

  // A rotating writer reports every segment it opens through its callback, this one included.
  if (properties->rotateIntervalBytes == 0) {
    recordingSegmentPaths_.push_back(filePath_);
  }

  fileOutputConfigured_.store(true, std::memory_order_release);
  return Result<NoneType, std::string>::Ok(None);
}

/// @brief Sets the callback to be invoked when audio data is ready.
/// If the recorder is already active, it prepares the callback for receiving audio data
/// immediately.
/// This method should be called from the JS thread only.
/// @param sampleRate Desired sample rate for the callback audio data.
/// @param bufferLength Desired buffer length in frames for the callback audio data.
/// @param channelCount Number of channels for the callback audio data.
/// @param callbackId Identifier for the JS callback to be invoked.
/// @returns Success status or Error status with message.
Result<NoneType, std::string> AudioRecorder::setOnAudioReadyCallback(
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

  if (isIdle()) {
    return Result<NoneType, std::string>::Ok(None);
  }

  auto formatResult = resolveStreamFormat();

  // The input is unavailable only transiently, so keep the callback registered: the next
  // start() prepares it against whatever format the input comes back with.
  if (!formatResult.is_ok()) {
    return Result<NoneType, std::string>::Err(formatResult.unwrap_err());
  }

  const auto format = formatResult.unwrap();
  auto prepareResult = dataCallback_->prepare(
      format.sampleRate, format.channelCount, static_cast<size_t>(format.maxFramesPerBuffer));

  if (!prepareResult.is_ok()) {
    callbackOutputEnabled_.store(false, std::memory_order_release);
    callbackOutputConfigured_.store(false, std::memory_order_release);
    dataCallback_ = nullptr;
    return Result<NoneType, std::string>::Err(prepareResult.unwrap_err());
  }

  callbackOutputConfigured_.store(true, std::memory_order_release);
  return Result<NoneType, std::string>::Ok(None);
}

/// @brief Clears the audio data callback.
/// If the recorder is currently active, it stops invoking the callback immediately.
/// This method should be called from the JS thread only.
void AudioRecorder::clearOnAudioReadyCallback() {
  std::scoped_lock callbackLock(callbackMutex_);
  callbackOutputConfigured_.store(false, std::memory_order_release);
  callbackOutputEnabled_.store(false, std::memory_order_release);
  dataCallback_ = nullptr;
}

/// @brief Connects a RecorderAdapterNode to the recorder for audio data routing.
/// If the recorder is already active, it initializes the adapter node immediately.
/// This method should be called from the JS thread only.
/// @param node Handle of the RecorderAdapterNode to connect.
void AudioRecorder::connect(const std::shared_ptr<utils::graph::NodeHandle> &node) {
  std::scoped_lock adapterLock(adapterNodeMutex_);
  adapterNodeHandle_ = node;
  isConnected_.store(true, std::memory_order_release);
  connectedConfigured_.store(false, std::memory_order_release);

  if (isIdle()) {
    return;
  }

  auto formatResult = resolveStreamFormat();

  if (!formatResult.is_ok()) {
    return;
  }

  prepareAdapterNode(formatResult.unwrap());
}

/// @brief Disconnects the currently connected RecorderAdapterNode from the recorder.
/// If the recorder is currently active, it stops routing audio data immediately.
/// This method should be called from the JS thread only.
void AudioRecorder::disconnect() {
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

void AudioRecorder::prepareAdapterNode(const StreamFormat &format) {
  if (adapterNodeHandle_ == nullptr) {
    return;
  }

  const auto maxFramesPerBuffer = static_cast<size_t>(format.maxFramesPerBuffer);
  // The shared fan-out deinterleaves through this before writing to the adapter node.
  deinterleavingBuffer_ =
      std::make_shared<AudioBuffer>(maxFramesPerBuffer, format.channelCount, format.sampleRate);
  static_cast<RecorderAdapterNode *>(adapterNodeHandle_->audioNode.get())
      ->init(maxFramesPerBuffer, format.channelCount, format.sampleRate);
  connectedConfigured_.store(true, std::memory_order_release);
}

AudioRecorder::DetachedOutputs AudioRecorder::detachOutputs() {
  DetachedOutputs outputs;
  const bool hadFileOutput = usesFileOutput();

  if (hadFileOutput) {
    fileOutputConfigured_.store(false, std::memory_order_release);
    outputs.fileWriter = std::move(fileWriter_);
  }

  if (usesCallback()) {
    callbackOutputConfigured_.store(false, std::memory_order_release);
    // Kept registered rather than moved out, so a later start() can re-prepare it.
    outputs.dataCallback = dataCallback_;
  }

  if (isConnected()) {
    connectedConfigured_.store(false, std::memory_order_release);
    outputs.adapterNodeHandle = std::move(adapterNodeHandle_);
  }

  for (const auto &segmentPath : recordingSegmentPaths_) {
    if (!segmentPath.empty()) {
      outputs.fileUris.push_back("file://" + segmentPath);
    }
  }

  // A writer that reported no segment still wrote the file it was opened with.
  if (hadFileOutput && outputs.fileUris.empty() && !filePath_.empty()) {
    outputs.fileUris.push_back("file://" + filePath_);
  }

  recordingSegmentPaths_.clear();
  filePath_ = "";

  return outputs;
}

AudioRecorder::StopResult AudioRecorder::finalizeOutputs(DetachedOutputs &&outputs) {
  double outputFileSize = 0.0;
  double outputDuration = 0.0;
  auto movedOutputs = std::move(outputs);

  if (movedOutputs.fileWriter != nullptr) {
    auto fileResult = movedOutputs.fileWriter->closeFile();

    if (!fileResult.is_ok()) {
      return StopResult::Err("Failed to close file: " + fileResult.unwrap_err());
    }

    outputFileSize = std::get<0>(fileResult.unwrap());
    outputDuration = std::get<1>(fileResult.unwrap());
  }

  if (movedOutputs.dataCallback != nullptr) {
    movedOutputs.dataCallback->cleanup();
  }

  if (movedOutputs.adapterNodeHandle != nullptr) {
    static_cast<RecorderAdapterNode *>(movedOutputs.adapterNodeHandle->audioNode.get())
        ->adapterCleanup();
  }

  return StopResult::Ok(
      std::make_tuple(std::move(movedOutputs.fileUris), outputFileSize, outputDuration));
}

/// @brief Sets the error callback to be invoked when an error occurs during recording.
/// This method should be called from the JS thread only.
/// @param callbackId Identifier for the JS callback to be invoked.
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

/// @brief Clears the error callback.
/// If the recorder is currently active, it will stop invoking the callback immediately.
/// This method should be called from the JS thread only.
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

/// @brief Gets the current duration of the recorded audio in seconds.
/// @returns Duration in seconds.
double AudioRecorder::getCurrentDuration() const {
  std::scoped_lock lock(fileWriterMutex_);
  double duration = 0.0;

  if (usesFileOutput() && fileWriter_ != nullptr) {
    duration = fileWriter_->getCurrentDuration();
  }

  return duration;
}

bool AudioRecorder::usesCallback() const {
  return wantsCallback() && callbackOutputConfigured_.load(std::memory_order_acquire);
}

bool AudioRecorder::usesFileOutput() const {
  return wantsFileOutput() && fileOutputConfigured_.load(std::memory_order_acquire);
}

bool AudioRecorder::isConnected() const {
  return wantsConnection() && connectedConfigured_.load(std::memory_order_acquire);
}

bool AudioRecorder::wantsCallback() const {
  return callbackOutputEnabled_.load(std::memory_order_acquire);
}

bool AudioRecorder::wantsFileOutput() const {
  return fileOutputEnabled_.load(std::memory_order_acquire);
}

bool AudioRecorder::wantsConnection() const {
  return isConnected_.load(std::memory_order_acquire);
}

} // namespace audioapi
// NOLINTEND(cppcoreguidelines-pro-type-static-cast-downcast)
