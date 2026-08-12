#include <audioapi/core/utils/EncodedAudioFileWriter.h>
#include <audioapi/encoding/EncoderCapabilities.h>
#include <audioapi/encoding/OSEncoding.h>
#include <audioapi/encoding/OSFilePath.h>
#include <audioapi/encoding/StreamFormat.h>
#include <audioapi/events/IAudioEventHandlerRegistry.h>
#include <audioapi/utils/AudioFileProperties.h>

#include <sys/stat.h>
#include <cstdio>
#include <cstring>
#include <memory>
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

  auto specResult = EncoderCapabilities::resolveOutputSpec(fileProperties_->format);
  if (specResult.is_err()) {
    return OpenFileResult::Err(specResult.unwrap_err());
  }
  const auto &outputSpec = specResult.unwrap();

  auto filePathResult = resolveOsFilePath(fileProperties_, fileNameOverride);
  if (filePathResult.is_err()) {
    return OpenFileResult::Err(filePathResult.unwrap_err());
  }
  filePath_ = filePathResult.unwrap();

  const StreamFormat inputFormat{
      .sampleRate = streamSampleRate,
      .channelCount = streamChannelCount,
  };

  auto encoder = createOsEncoder(fileProperties_);
  if (encoder == nullptr) {
    return OpenFileResult::Err("Audio file recording requires iOS or Android.");
  }
  auto openResult =
      encoder->open(inputFormat, outputSpec, static_cast<size_t>(maxFramesPerBuffer), filePath_);
  if (openResult.is_err()) {
    return OpenFileResult::Err(openResult.unwrap_err());
  }
  encoder_ = std::move(encoder);

  if (!initializePreallocatedInputPool()) {
    rollbackFailedOpen();
    return OpenFileResult::Err("Failed to preallocate file writer buffers");
  }

  isFileOpen_.store(true, std::memory_order_release);
  return OpenFileResult::Ok(filePath_);
}

void EncodedAudioFileWriter::rollbackFailedOpen() {
  cleanupPreallocatedInputPool();
  if (encoder_ != nullptr) {
    encoder_->close();
    encoder_.reset();
  }
  if (!filePath_.empty()) {
    std::remove(filePath_.c_str());
    filePath_ = "";
  }
  framesWritten_.store(0, std::memory_order_release);
  isFileOpen_.store(false, std::memory_order_release);
}

/// @brief Finalizes the output file. Called from the JS thread only.
CloseFileResult EncodedAudioFileWriter::closeFile() {
  if (!isFileOpen() || encoder_ == nullptr) {
    return CloseFileResult::Err("file is not open: " + filePath_);
  }

  isFileOpen_.store(false, std::memory_order_release);

  // Drains the worker queue before finalizing the encoder below.
  cleanupPreallocatedInputPool();

  auto closeResult = encoder_->close();
  encoder_.reset();
  framesWritten_.store(0, std::memory_order_release);
  filePath_ = "";

  if (closeResult.is_err()) {
    return CloseFileResult::Err(closeResult.unwrap_err());
  }
  return CloseFileResult::Ok(closeResult.unwrap());
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

  // Recreated on every open (closeFile tears it down), last so it never sees a half-built pool.
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

  if (isFileOpen() && encoder_ != nullptr) {
    auto result = encoder_->encode(inputBufferPool_.get() + slot * samplesPerSlot_, numFrames);
    if (result.is_ok()) {
      framesWritten_.fetch_add(numFrames, std::memory_order_acq_rel);
    } else {
      invokeOnErrorCallback(
          "Failed to write audio data to file: " + filePath_ + " - " + result.unwrap_err());
    }
  }

  freeSlots_->release(slot);
}

std::string EncodedAudioFileWriter::getFilePath() const {
  return filePath_;
}

double EncodedAudioFileWriter::getCurrentDuration() const {
  const double sampleRate = streamSampleRate_ > 0 ? streamSampleRate_ : fileProperties_->sampleRate;
  if (sampleRate <= 0) {
    return 0.0;
  }
  return static_cast<double>(framesWritten_.load(std::memory_order_acquire)) / sampleRate;
}

size_t EncodedAudioFileWriter::getFileSizeBytes() const {
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
