#include <audioapi/core/inputs/AudioRecorder.h>

#include <audioapi/core/sources/RecorderAdapterNode.h>
#include <audioapi/core/utils/AudioFileWriter.h>
#include <audioapi/core/utils/AudioRecorderCallback.h>
#include <audioapi/core/utils/Locker.h>
#include <audioapi/utils/AudioFileProperties.h>
#include <audioapi/utils/CircularOverflowableAudioArray.h>

#include <algorithm>
#include <memory>
#include <string>
#include <utility>

// NOLINTBEGIN(cppcoreguidelines-pro-type-static-cast-downcast)

namespace audioapi {

void AudioRecorder::onAudioFrames(const float *interleavedFrames, int numFrames) {
  if (interleavedFrames == nullptr || numFrames <= 0) {
    return;
  }

  lastCallbackFrameCount_.store(numFrames, std::memory_order_release);

  if (usesFileOutput()) {
    auto fileWriterLock = Locker::tryLock(fileWriterMutex_);
    if (fileWriterLock && fileWriter_) {
      fileWriter_->writeAudioData(interleavedFrames, numFrames);
    }
  }

  if (usesCallback()) {
    auto callbackLock = Locker::tryLock(callbackMutex_);
    if (callbackLock && dataCallback_) {
      dataCallback_->receiveAudioData(interleavedFrames, numFrames);
    }
  }

  if (isConnected()) {
    auto adapterLock = Locker::tryLock(adapterNodeMutex_);
    if (!adapterLock || !adapterNodeHandle_ || !deinterleavingBuffer_) {
      return;
    }
    // A callback larger than the stream's maximum burst would overrun the buffer.
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

/// JS thread only. The file itself is created by the next start(). An active (recording or
/// paused) session keeps the output it started with, so calling this during a session fails and
/// changes nothing.
Result<NoneType, std::string> AudioRecorder::enableFileOutput(
    std::shared_ptr<AudioFileProperties> properties) {
  std::scoped_lock fileWriterLock(fileWriterMutex_, errorCallbackMutex_);

  if (!isIdle()) {
    return Result<NoneType, std::string>::Err(
        "File output cannot be changed while a recording session is active");
  }

  fileProperties_ = std::move(properties);
  fileOutputEnabled_.store(true, std::memory_order_release);
  fileOutputConfigured_.store(false, std::memory_order_release);

  return Result<NoneType, std::string>::Ok(None);
}

/// JS thread only. Closes the file immediately when called mid-recording.
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

Result<NoneType, std::string> AudioRecorder::setupFileWriter(
    const std::shared_ptr<AudioFileProperties> &properties) {
  auto formatResult = resolveStreamFormat();

  if (!formatResult.is_ok()) {
    return Result<NoneType, std::string>::Err(
        "Failed to open file for writing: " + formatResult.unwrap_err());
  }

  fileWriter_ = std::make_shared<AudioFileWriter>(
      audioEventHandlerRegistry_, properties, [this](const std::string &path) {
        std::scoped_lock lock(segmentPathsMutex_);
        recordingSegmentPaths_.push_back(path);
      });
  fileWriter_->setOnErrorCallback(errorCallbackId_.load(std::memory_order_acquire));

  const auto format = formatResult.unwrap();
  auto fileResult =
      fileWriter_->openFile(format.sampleRate, format.channelCount, format.maxFramesPerBuffer);

  if (!fileResult.is_ok()) {
    fileOutputConfigured_.store(false, std::memory_order_release);
    fileWriter_ = nullptr;
    return Result<NoneType, std::string>::Err(
        "Failed to open file for writing: " + fileResult.unwrap_err());
  }

  filePath_ = fileResult.unwrap();
  fileOutputConfigured_.store(true, std::memory_order_release);
  return Result<NoneType, std::string>::Ok(None);
}

/// JS thread only. Prepares the callback immediately when called mid-recording.
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

/// JS thread only.
void AudioRecorder::clearOnAudioReadyCallback() {
  std::scoped_lock callbackLock(callbackMutex_);
  callbackOutputConfigured_.store(false, std::memory_order_release);
  callbackOutputEnabled_.store(false, std::memory_order_release);
  dataCallback_ = nullptr;
}

/// JS thread only. Prepares the node immediately when called mid-recording.
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

/// JS thread only.
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
  deinterleavingBuffer_ =
      std::make_shared<AudioBuffer>(maxFramesPerBuffer, format.channelCount, format.sampleRate);
  static_cast<RecorderAdapterNode *>(adapterNodeHandle_->audioNode.get())
      ->init(maxFramesPerBuffer, format.channelCount, format.sampleRate);
  connectedConfigured_.store(true, std::memory_order_release);
}

AudioRecorder::DetachedSideEffects AudioRecorder::detachSideEffects() {
  DetachedSideEffects sideEffects;

  if (usesFileOutput()) {
    fileOutputConfigured_.store(false, std::memory_order_release);
    sideEffects.fileWriter = std::move(fileWriter_);
  }

  if (usesCallback()) {
    callbackOutputConfigured_.store(false, std::memory_order_release);
    // Kept registered rather than moved out, so a later start() can re-prepare it.
    sideEffects.dataCallback = dataCallback_;
  }

  if (isConnected()) {
    connectedConfigured_.store(false, std::memory_order_release);
    sideEffects.adapterNodeHandle = std::move(adapterNodeHandle_);
  }

  filePath_ = "";

  return sideEffects;
}

AudioRecorder::StopResult AudioRecorder::finalizeSideEffects(DetachedSideEffects &&sideEffects) {
  double outputFileSize = 0.0;
  double outputDuration = 0.0;
  auto movedSideEffects = std::move(sideEffects);

  if (movedSideEffects.fileWriter != nullptr) {
    auto fileResult = movedSideEffects.fileWriter->closeFile();

    if (!fileResult.is_ok()) {
      return StopResult::Err("Failed to close file: " + fileResult.unwrap_err());
    }

    outputFileSize = std::get<0>(fileResult.unwrap());
    outputDuration = std::get<1>(fileResult.unwrap());

    std::scoped_lock lock(segmentPathsMutex_);
    for (const auto &segmentPath : recordingSegmentPaths_) {
      if (!segmentPath.empty()) {
        movedSideEffects.fileUris.push_back("file://" + segmentPath);
      }
    }
    recordingSegmentPaths_.clear();
  }

  if (movedSideEffects.dataCallback != nullptr) {
    movedSideEffects.dataCallback->cleanup();
  }

  if (movedSideEffects.adapterNodeHandle != nullptr) {
    static_cast<RecorderAdapterNode *>(movedSideEffects.adapterNodeHandle->audioNode.get())
        ->adapterCleanup();
  }

  return StopResult::Ok(
      std::make_tuple(std::move(movedSideEffects.fileUris), outputFileSize, outputDuration));
}

/// JS thread only.
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

/// JS thread only.
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

double AudioRecorder::getCurrentDuration() const {
  double duration = 0.0;

  if (usesFileOutput() && fileWriter_ != nullptr) {
    duration = fileWriter_->getCurrentDuration();
  }

  return duration;
}

RecorderState AudioRecorder::getState() const {
  if (isIdle()) {
    return RecorderState::Idle;
  }

  return isPaused() ? RecorderState::Paused : RecorderState::Recording;
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
