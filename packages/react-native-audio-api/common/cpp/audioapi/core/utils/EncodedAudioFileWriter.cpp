#include <audioapi/core/utils/EncodedAudioFileWriter.h>
#include <audioapi/encoding/EncoderCapabilities.h>
#include <audioapi/encoding/OSEncoding.h>
#include <audioapi/encoding/OSFilePath.h>
#include <audioapi/encoding/StreamFormat.h>
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
#include <utility>

namespace audioapi {

EncodedAudioFileWriter::EncodedAudioFileWriter(
    const std::shared_ptr<IAudioEventHandlerRegistry> &audioEventHandlerRegistry,
    const std::shared_ptr<AudioFileProperties> &fileProperties)
    : AudioFileWriter(audioEventHandlerRegistry, fileProperties) {}

EncodedAudioFileWriter::~EncodedAudioFileWriter() {
  isFileOpen_.store(false, std::memory_order_release);
  cleanupPreallocatedInputPool();
  encoder_.reset();
}

/// @brief Resolves the output path, opens the system encoder and preallocates the
/// audio-thread → worker buffer pool. Called from the JS thread only.
OpenFileResult EncodedAudioFileWriter::openFile(
    float streamSampleRate,
    int32_t streamChannelCount,
    int32_t maxFramesPerBuffer,
    const std::string &fileNameOverride) {
  if (encoder_ != nullptr) {
    return OpenFileResult::Err("file already open");
  }

  streamSampleRate_ = streamSampleRate;
  streamChannelCount_ = streamChannelCount;
  maxFramesPerBuffer_ = maxFramesPerBuffer;
  framesWritten_.store(0, std::memory_order_release);

  if (fileProperties_->sampleRate <= 0 || fileProperties_->channelCount <= 0) {
    return OpenFileResult::Err(
        "Invalid file properties: sampleRate and channelCount must be greater than 0");
  }
  if (streamSampleRate <= 0 || streamChannelCount <= 0 || maxFramesPerBuffer <= 0) {
    return OpenFileResult::Err(
        "Invalid input format: sampleRate, channelCount and buffer size must be greater than 0");
  }

  OpenFileResult openResult = OpenFileResult::Err("");
  {
    std::scoped_lock lock(fileMutex_);
    openResult = openEncoderForFile(fileNameOverride);
  }
  if (openResult.is_err()) {
    return openResult;
  }

  if (!initializePreallocatedInputPool()) {
    rollbackFailedOpen();
    return OpenFileResult::Err("Failed to preallocate file writer buffers");
  }

  isFileOpen_.store(true, std::memory_order_release);
  return openResult;
}

/// @brief Resolves the output path and opens an encoder on it. The caller must hold fileMutex_.
OpenFileResult EncodedAudioFileWriter::openEncoderForFile(const std::string &fileNameOverride) {
  auto specResult = EncoderCapabilities::resolveOutputSpec(fileProperties_->format);
  if (specResult.is_err()) {
    return OpenFileResult::Err(specResult.unwrap_err());
  }
  const auto &outputSpec = specResult.unwrap();

  auto filePathResult = resolveOsFilePath(
      fileProperties_, fileNameOverride + "." + std::string(outputSpec.extension));
  if (filePathResult.is_err()) {
    return OpenFileResult::Err(filePathResult.unwrap_err());
  }
  filePath_ = filePathResult.unwrap();

  struct stat existing{};
  if (::stat(filePath_.c_str(), &existing) == 0) {
#ifdef ANDROID
    __android_log_print(
        ANDROID_LOG_WARN,
        "RN_AUDIOAPI",
        "recording overwrites an existing file: %s",
        filePath_.c_str());
#else
    printf("[RN_AUDIOAPI WARN] recording overwrites an existing file: %s\n", filePath_.c_str());
#endif
  }

  const StreamFormat inputFormat{
      .sampleRate = streamSampleRate_,
      .channelCount = streamChannelCount_,
  };

  auto encoder = createOsEncoder(fileProperties_);
  if (encoder == nullptr) {
    return OpenFileResult::Err("Audio file recording requires iOS or Android.");
  }
  auto openResult =
      encoder->open(inputFormat, outputSpec, static_cast<size_t>(maxFramesPerBuffer_), filePath_);
  if (openResult.is_err()) {
    return OpenFileResult::Err(openResult.unwrap_err());
  }
  encoder_ = std::move(encoder);
  framesWritten_.store(0, std::memory_order_release);

  return OpenFileResult::Ok(filePath_);
}

CloseEncoderResult EncodedAudioFileWriter::retireEncoder() {
  if (encoder_ == nullptr) {
    return CloseEncoderResult::Err("file is not open: " + filePath_);
  }

  auto closeResult = encoder_->close();
  encoder_.reset();
  framesWritten_.store(0, std::memory_order_release);
  return closeResult;
}

void EncodedAudioFileWriter::rollbackFailedOpen() {
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

/// @brief Finalizes the output file. Called from the JS thread only.
CloseFileResult EncodedAudioFileWriter::closeFile() {
  if (!isFileOpen()) {
    return CloseFileResult::Err("file is not open: " + getFilePath());
  }

  // Drains and joins the worker while the file still counts as open, so buffers already
  // queued are encoded rather than dropped. Must run without fileMutex_: the worker takes
  // it, and joining while holding it would deadlock.
  cleanupPreallocatedInputPool();
  isFileOpen_.store(false, std::memory_order_release);

  std::scoped_lock lock(fileMutex_);
  auto closeResult = retireEncoder();
  filePath_ = "";

  if (closeResult.is_err()) {
    return CloseFileResult::Err(closeResult.unwrap_err());
  }
  return CloseFileResult::Ok(closeResult.unwrap());
}

/// @brief Retires the current file and continues into the next one, leaving the worker thread
/// and the buffer pool in place. Called from the worker thread, by way of the buffer-encoded
/// listener, so that closing and opening an encoder never lands on the audio thread.
CloseFileResult EncodedAudioFileWriter::switchToFile(const std::string &fileNameOverride) {
  std::scoped_lock lock(fileMutex_);
  auto closeResult = retireEncoder();
  if (closeResult.is_err()) {
    return CloseFileResult::Err(closeResult.unwrap_err());
  }

  auto openResult = openEncoderForFile(fileNameOverride);
  if (openResult.is_err()) {
    // Nothing more can be encoded, so stop the audio thread from queueing into a dead writer.
    isFileOpen_.store(false, std::memory_order_release);
    return CloseFileResult::Err(openResult.unwrap_err());
  }

  return CloseFileResult::Ok(closeResult.unwrap());
}

void EncodedAudioFileWriter::setOnBufferEncodedCallback(std::function<void()> callback) {
  onBufferEncoded_ = std::move(callback);
}

void EncodedAudioFileWriter::createOffloader() {
  auto offloaderLambda = [this](PendingFileWrite pending) {
    runWriterTask(pending);
  };
  offloader_ = std::make_unique<Offloader>(FILE_WRITER_CHANNEL_CAPACITY, offloaderLambda);
}

bool EncodedAudioFileWriter::initializePreallocatedInputPool() {
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

  // Last, so the worker never sees a half-built pool. It then lives for the whole session:
  // switchToFile() swaps the encoder underneath it rather than replacing it.
  createOffloader();
  return true;
}

void EncodedAudioFileWriter::cleanupPreallocatedInputPool() {
  // Stop the worker before freeing the pool/free list it accesses.
  offloader_.reset();
  freeSlots_.reset();
  inputBufferPool_.reset();
  samplesPerSlot_ = 0;
}

void EncodedAudioFileWriter::writeAudioData(const float *interleavedFrames, int numFrames) {
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

  // The recorder owns `interleavedFrames` only for the duration of this synchronous
  // callback. Copy into an owned slot before handing off to the worker thread; the
  // consumer in runWriterTask releases the slot.
  std::memcpy(
      inputBufferPool_.get() + slot.value() * samplesPerSlot_,
      interleavedFrames,
      samples * sizeof(float));
  // send() cannot block here: we hold a slot from a pool of FILE_WRITER_POOL_SIZE,
  // and the channel is sized one larger, so the ring always has room while any slot
  // is in flight.
  offloader_->getSender()->send(PendingFileWrite{.slot = slot.value(), .numFrames = numFrames});
}

void EncodedAudioFileWriter::runWriterTask(PendingFileWrite pending) {
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
  }

  // Outside the lock so the listener may call straight back in — rotation does exactly that,
  // reading the file size and then switching us onto the next segment.
  if (encoded) {
    notifyBufferEncoded();
  }
}

void EncodedAudioFileWriter::notifyBufferEncoded() {
  if (onBufferEncoded_) {
    onBufferEncoded_();
  }
}

std::string EncodedAudioFileWriter::getFilePath() const {
  std::scoped_lock lock(fileMutex_);
  return filePath_;
}

double EncodedAudioFileWriter::getCurrentDuration() const {
  std::scoped_lock lock(fileMutex_);
  const double sampleRate = streamSampleRate_ > 0 ? streamSampleRate_ : fileProperties_->sampleRate;
  if (sampleRate <= 0) {
    return 0.0;
  }
  return static_cast<double>(framesWritten_.load(std::memory_order_acquire)) / sampleRate;
}

size_t EncodedAudioFileWriter::getFileSizeBytes() const {
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

} // namespace audioapi
