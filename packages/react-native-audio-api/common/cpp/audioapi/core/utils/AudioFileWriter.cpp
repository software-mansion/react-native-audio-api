#include <audioapi/core/utils/AudioFileWriter.h>
#include <audioapi/core/utils/RecordingFileName.h>
#include <audioapi/encoding/EncoderCapabilities.h>
#include <audioapi/encoding/OSEncoding.h>
#include <audioapi/encoding/OSFilePath.h>
#include <audioapi/encoding/StreamFormat.h>
#include <audioapi/events/AudioEventPayload.h>
#include <audioapi/events/IAudioEventHandlerRegistry.h>
#include <audioapi/utils/AudioFileProperties.h>

#ifdef ANDROID
#include <android/log.h>
#endif

#include <sys/stat.h>
#include <cstdio>
#include <cstring>
#include <memory>
#include <mutex>
#include <new>
#include <string>
#include <tuple>
#include <utility>

namespace audioapi {

PlatformFileBackend createOsFileBackend() {
  return PlatformFileBackend{
      .resolvePath = &resolveOsFilePath,
      .createEncoder = &createOsEncoder,
  };
}

AudioFileWriter::AudioFileWriter(
    const std::shared_ptr<IAudioEventHandlerRegistry> &audioEventHandlerRegistry,
    const std::shared_ptr<AudioFileProperties> &fileProperties,
    OnFileOpenedCallback onFileOpened,
    PlatformFileBackend backend)
    : fileProperties_(fileProperties),
      onFileOpened_(std::move(onFileOpened)),
      backend_(std::move(backend)),
      errorEvent_(audioEventHandlerRegistry) {}

AudioFileWriter::~AudioFileWriter() {
  isFileOpen_.store(false, std::memory_order_release);
  cleanupPreallocatedInputPool();
  encoder_.reset();
}

OpenFileResult AudioFileWriter::openFile(
    float streamSampleRate,
    int32_t streamChannelCount,
    int32_t maxFramesPerBuffer) {
  if (isFileOpen()) {
    return OpenFileResult::Err("file already open");
  }

  {
    std::scoped_lock lock(fileMutex_);
    sessionStem_ = recordingfilename::sessionStem(fileProperties_);
    openedFileCount_ = 0;
    finishedFilesSizeMB_ = 0.0;
    finishedFilesDurationSec_ = 0.0;
  }

  return startNextFile(streamSampleRate, streamChannelCount, maxFramesPerBuffer);
}

CloseFileResult AudioFileWriter::closeFile() {
  if (!isFileOpen()) {
    return CloseFileResult::Err("file is not open: " + getFilePath());
  }

  auto finished = finishCurrentFile();
  if (finished.is_err()) {
    return CloseFileResult::Err(finished.unwrap_err());
  }

  std::scoped_lock lock(fileMutex_);
  const double sessionSizeMB = finishedFilesSizeMB_;
  const double sessionDurationSec = finishedFilesDurationSec_;
  finishedFilesSizeMB_ = 0.0;
  finishedFilesDurationSec_ = 0.0;
  return CloseFileResult::Ok({sessionSizeMB, sessionDurationSec});
}

OpenFileResult AudioFileWriter::reprepareStreamFormat(
    float streamSampleRate,
    int32_t streamChannelCount,
    int32_t maxFramesPerBuffer) {
  if (!isFileOpen()) {
    return OpenFileResult::Err("file is not open");
  }

  // A file that failed to close is left out of the totals; the session still moves on.
  finishCurrentFile();

  return startNextFile(streamSampleRate, streamChannelCount, maxFramesPerBuffer);
}

OpenFileResult AudioFileWriter::startNextFile(
    float streamSampleRate,
    int32_t streamChannelCount,
    int32_t maxFramesPerBuffer) {
  if (fileProperties_->sampleRate <= 0 || fileProperties_->channelCount <= 0) {
    return OpenFileResult::Err(
        "Invalid file properties: sampleRate and channelCount must be greater than 0");
  }
  if (streamSampleRate <= 0 || streamChannelCount <= 0 || maxFramesPerBuffer <= 0) {
    return OpenFileResult::Err(
        "Invalid input format: sampleRate, channelCount and buffer size must be greater than 0");
  }

  streamSampleRate_ = streamSampleRate;
  streamChannelCount_ = streamChannelCount;
  maxFramesPerBuffer_ = maxFramesPerBuffer;
  framesWritten_.store(0, std::memory_order_release);

  OpenFileResult openResult = OpenFileResult::Err("");
  {
    std::scoped_lock lock(fileMutex_);
    openResult = openEncoderForNextFile();
  }
  if (openResult.is_err()) {
    return openResult;
  }

  if (!initializePreallocatedInputPool()) {
    rollbackFailedOpen();
    return OpenFileResult::Err("Failed to preallocate file writer buffers");
  }

  isFileOpen_.store(true, std::memory_order_release);
  announceFileOpened(openResult.unwrap());
  return openResult;
}

CloseEncoderResult AudioFileWriter::finishCurrentFile() {
  // Drains and joins the worker while the file still counts as open, so buffers already
  // queued are encoded rather than dropped. Must run without fileMutex_: the worker takes
  // it, and joining while holding it would deadlock.
  cleanupPreallocatedInputPool();
  isFileOpen_.store(false, std::memory_order_release);

  std::scoped_lock lock(fileMutex_);
  auto closeResult = retireEncoder();
  filePath_ = "";

  if (closeResult.is_ok()) {
    foldFinishedFile(closeResult.unwrap());
  }
  return closeResult;
}

std::string AudioFileWriter::fileStem(size_t fileNumber) const {
  if (rotatesFiles()) {
    return recordingfilename::segmentStem(sessionStem_, fileNumber);
  }
  if (fileNumber == 1) {
    return sessionStem_;
  }
  return recordingfilename::segmentStem(sessionStem_, fileNumber - 1);
}

OpenFileResult AudioFileWriter::openEncoderForNextFile() {
  // Calling an empty std::function throws, which on the worker thread would terminate.
  if (!backend_.resolvePath || !backend_.createEncoder) {
    return OpenFileResult::Err("File writer was constructed without a platform backend");
  }

  auto specResult = EncoderCapabilities::resolveOutputSpec(fileProperties_->format);
  if (specResult.is_err()) {
    return OpenFileResult::Err(specResult.unwrap_err());
  }
  const auto &outputSpec = specResult.unwrap();

  const std::string fileName =
      fileStem(openedFileCount_ + 1) + "." + std::string(outputSpec.extension);
  auto filePathResult = backend_.resolvePath(fileProperties_, fileName);
  if (filePathResult.is_err()) {
    return OpenFileResult::Err(filePathResult.unwrap_err());
  }
  const std::string &filePath = filePathResult.unwrap();

  struct stat existing{};
  if (::stat(filePath.c_str(), &existing) == 0) {
#ifdef ANDROID
    __android_log_print(
        ANDROID_LOG_WARN,
        "RN_AUDIOAPI",
        "recording overwrites an existing file: %s",
        filePath.c_str());
#else
    printf("[RN_AUDIOAPI WARN] recording overwrites an existing file: %s\n", filePath.c_str());
#endif
  }

  const StreamFormat inputFormat{
      .sampleRate = streamSampleRate_,
      .channelCount = streamChannelCount_,
  };

  auto encoder = backend_.createEncoder(fileProperties_);
  if (encoder == nullptr) {
    return OpenFileResult::Err("Audio file recording requires iOS or Android.");
  }
  auto openResult =
      encoder->open(inputFormat, outputSpec, static_cast<size_t>(maxFramesPerBuffer_), filePath);
  if (openResult.is_err()) {
    return OpenFileResult::Err(openResult.unwrap_err());
  }

  encoder_ = std::move(encoder);
  filePath_ = filePath;
  ++openedFileCount_;
  writesSinceLastSizeCheck_ = 0;
  framesWritten_.store(0, std::memory_order_release);

  return OpenFileResult::Ok(filePath_);
}

CloseEncoderResult AudioFileWriter::retireEncoder() {
  if (encoder_ == nullptr) {
    return CloseEncoderResult::Err("file is not open: " + filePath_);
  }

  auto closeResult = encoder_->close();
  encoder_.reset();
  framesWritten_.store(0, std::memory_order_release);
  return closeResult;
}

void AudioFileWriter::foldFinishedFile(const std::tuple<double, double> &finished) {
  finishedFilesSizeMB_ += std::get<0>(finished);
  finishedFilesDurationSec_ += std::get<1>(finished);
}

void AudioFileWriter::rollbackFailedOpen() {
  cleanupPreallocatedInputPool();

  std::scoped_lock lock(fileMutex_);
  // Whatever the encoder reports about a file that is about to be deleted is of no use.
  retireEncoder();
  if (!filePath_.empty()) {
    std::remove(filePath_.c_str());
    filePath_ = "";
  }
  isFileOpen_.store(false, std::memory_order_release);
}

void AudioFileWriter::rotateOnceFileOutgrowsCap() {
  if (!rotatesFiles()) {
    return;
  }

  std::string openedPath;
  std::string rotationError;
  {
    std::scoped_lock lock(fileMutex_);
    if (!isFileOpen() || encoder_ == nullptr) {
      return;
    }
    if (++writesSinceLastSizeCheck_ < FILE_SIZE_CHECK_WRITE_INTERVAL) {
      return;
    }
    writesSinceLastSizeCheck_ = 0;

    if (encoder_->getFileSizeBytes() <= fileProperties_->rotateIntervalBytes) {
      return;
    }

    auto closeResult = retireEncoder();
    if (closeResult.is_err()) {
      rotationError = closeResult.unwrap_err();
    } else {
      foldFinishedFile(closeResult.unwrap());
      auto openResult = openEncoderForNextFile();
      if (openResult.is_err()) {
        rotationError = openResult.unwrap_err();
      } else {
        openedPath = openResult.unwrap();
      }
    }

    if (!rotationError.empty()) {
      // Nothing more can be encoded, so stop the audio thread from queueing into a dead writer.
      isFileOpen_.store(false, std::memory_order_release);
    }
  }

  // Both reach back into the outside world, so neither runs under the lock.
  if (!rotationError.empty()) {
    invokeOnErrorCallback("Failed to start the next recording segment: " + rotationError);
    return;
  }
  announceFileOpened(openedPath);
}

void AudioFileWriter::announceFileOpened(const std::string &path) {
  if (onFileOpened_ && !path.empty()) {
    onFileOpened_(path);
  }
}

void AudioFileWriter::createOffloader() {
  auto offloaderLambda = [this](PendingFileWrite pending) {
    runWriterTask(pending);
  };
  offloader_ = std::make_unique<Offloader>(FILE_WRITER_CHANNEL_CAPACITY, offloaderLambda);
}

bool AudioFileWriter::initializePreallocatedInputPool() {
  cleanupPreallocatedInputPool();

  if (maxFramesPerBuffer_ <= 0 || streamChannelCount_ <= 0) {
    return false;
  }

  samplesPerSlot_ =
      static_cast<size_t>(maxFramesPerBuffer_) * static_cast<size_t>(streamChannelCount_);
  // nothrow new keeps the graceful failure path (return false) instead of throwing.
  inputBufferPool_.reset(new (std::nothrow) float[samplesPerSlot_ * FILE_WRITER_POOL_SIZE]);
  if (inputBufferPool_ == nullptr) {
    samplesPerSlot_ = 0;
    return false;
  }

  freeSlots_ = std::make_unique<FreeList>();
  freeSlots_->seed();

  // Last, so the worker never sees a half-built pool.
  createOffloader();
  return true;
}

void AudioFileWriter::cleanupPreallocatedInputPool() {
  // Stop the worker before freeing the pool/free list it accesses.
  offloader_.reset();
  freeSlots_.reset();
  inputBufferPool_.reset();
  samplesPerSlot_ = 0;
}

void AudioFileWriter::writeAudioData(const float *interleavedFrames, int numFrames) {
  if (!isFileOpen() || interleavedFrames == nullptr || offloader_ == nullptr ||
      freeSlots_ == nullptr || inputBufferPool_ == nullptr || samplesPerSlot_ == 0) {
    return;
  }

  auto slot = freeSlots_->tryAcquire();
  if (!slot.has_value()) {
    return;
  }

  const size_t samples = static_cast<size_t>(numFrames) * static_cast<size_t>(streamChannelCount_);
  if (samples > samplesPerSlot_) {
    freeSlots_->release(slot.value());
    return;
  }

  // runWriterTask releases the slot.
  std::memcpy(
      inputBufferPool_.get() + slot.value() * samplesPerSlot_,
      interleavedFrames,
      samples * sizeof(float));
  // Never blocks: the channel has room for every slot the pool can hand out.
  offloader_->getSender()->send(PendingFileWrite{.slot = slot.value(), .numFrames = numFrames});
}

void AudioFileWriter::runWriterTask(PendingFileWrite pending) {
  auto [slot, numFrames] = pending;
  if (slot == FreeList::kSentinel) {
    return;
  }
  if (slot >= FILE_WRITER_POOL_SIZE || freeSlots_ == nullptr || inputBufferPool_ == nullptr) {
    return;
  }

  std::string encodeError;
  bool encoded = false;
  {
    std::scoped_lock lock(fileMutex_);
    if (isFileOpen() && encoder_ != nullptr) {
      auto result = encoder_->encode(inputBufferPool_.get() + slot * samplesPerSlot_, numFrames);
      if (result.is_ok()) {
        framesWritten_.fetch_add(numFrames, std::memory_order_acq_rel);
        encoded = true;
      } else {
        encodeError =
            "Failed to write audio data to file: " + filePath_ + " - " + result.unwrap_err();
      }
    }
  }

  freeSlots_->release(slot);

  if (!encodeError.empty()) {
    invokeOnErrorCallback(encodeError);
    return;
  }
  if (encoded) {
    rotateOnceFileOutgrowsCap();
  }
}

std::string AudioFileWriter::getFilePath() const {
  std::scoped_lock lock(fileMutex_);
  return filePath_;
}

double AudioFileWriter::getCurrentDuration() const {
  std::scoped_lock lock(fileMutex_);
  const double sampleRate = streamSampleRate_ > 0 ? streamSampleRate_ : fileProperties_->sampleRate;
  if (sampleRate <= 0) {
    return finishedFilesDurationSec_;
  }
  const double currentFileDurationSec =
      static_cast<double>(framesWritten_.load(std::memory_order_acquire)) / sampleRate;
  return finishedFilesDurationSec_ + currentFileDurationSec;
}

size_t AudioFileWriter::getFileSizeBytes() const {
  std::scoped_lock lock(fileMutex_);
  if (encoder_ != nullptr) {
    return encoder_->getFileSizeBytes();
  }
  struct stat st{};
  if (!filePath_.empty() && stat(filePath_.c_str(), &st) == 0) {
    return static_cast<size_t>(st.st_size);
  }
  return 0;
}

void AudioFileWriter::setOnErrorCallback(uint64_t callbackId) {
  errorEvent_.assignCallbackId(callbackId);
}

void AudioFileWriter::clearOnErrorCallback() {
  errorEvent_.assignCallbackId(0);
}

void AudioFileWriter::invokeOnErrorCallback(const std::string &message) {
  errorEvent_.dispatch(StringPayload{.name = "message", .reason = message});
}

bool AudioFileWriter::isFileOpen() const {
  return isFileOpen_.load(std::memory_order_acquire);
}

bool AudioFileWriter::rotatesFiles() const {
  return fileProperties_->rotateIntervalBytes > 0;
}

} // namespace audioapi
