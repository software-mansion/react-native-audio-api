#import <AVFoundation/AVFoundation.h>
#import <Foundation/Foundation.h>

#include <audioapi/encoding/EncoderCapabilities.h>
#include <audioapi/encoding/StreamFormat.h>
#include <audioapi/events/AudioEventHandlerRegistry.h>
#include <audioapi/ios/core/utils/FileOptions.h>
#include <audioapi/ios/core/utils/IOSEncoding.h>
#include <audioapi/ios/core/utils/IOSFileWriter.h>
#include <audioapi/ios/core/utils/OwnedAudioBufferList.h>
#include <audioapi/utils/AudioFileProperties.h>
#include <audioapi/utils/Result.hpp>
#include <utility>

namespace audioapi {

using ios::allocateOwnedAudioBufferList;
using ios::copyIntoOwnedAudioBufferList;
using ios::OwnedAudioBufferListPtr;

IOSFileWriter::IOSFileWriter(
    const std::shared_ptr<AudioEventHandlerRegistry> &audioEventHandlerRegistry,
    const std::shared_ptr<AudioFileProperties> &fileProperties)
    : AudioFileWriter(audioEventHandlerRegistry, fileProperties)
{
}

IOSFileWriter::~IOSFileWriter()
{
  @autoreleasepool {
    offloader_.reset();
    freeSlots_.reset();
    inputBufferPool_.clear();
    inputBufferBytesPerBuffer_ = 0;

    encoder_.reset();
    bufferFormat_ = nil;
  }
}

/// @brief Opens an audio file for writing with the specified buffer format and
/// maximum input buffer length. Resolves the output path, creates the encoder
/// and preallocates the audio-thread → worker buffer pool.
/// This method should be called from the JS thread only.
OpenFileResult IOSFileWriter::openFile(
    AVAudioFormat *bufferFormat,
    size_t maxInputBufferLength,
    const std::string &fileNameOverride)
{
  @autoreleasepool {
    if (encoder_ != nullptr) {
      return OpenFileResult::Err("file already open");
    }

    framesWritten_.store(0, std::memory_order_release);
    bufferFormat_ = bufferFormat;
    inputSampleRate_ = bufferFormat.sampleRate;

    if (fileProperties_->sampleRate == 0 || fileProperties_->channelCount == 0) {
      return OpenFileResult::Err(
          "Invalid file properties: sampleRate and channelCount must be greater than 0");
    }
    if (bufferFormat.sampleRate == 0 || bufferFormat.channelCount == 0) {
      return OpenFileResult::Err(
          "Invalid input format: sampleRate and channelCount must be greater than 0");
    }

    auto specResult = EncoderCapabilities::resolveOutputSpec(fileProperties_->format);
    if (specResult.is_err()) {
      return OpenFileResult::Err(specResult.unwrap_err());
    }
    EncoderOutputSpec outputSpec = specResult.unwrap();

    NSURL *fileURL = ios::fileoptions::getFileURL(fileProperties_, fileNameOverride);
    filePath_ = [[fileURL path] UTF8String];

    StreamFormat inputFormat{
        .sampleRate = static_cast<float>(bufferFormat.sampleRate),
        .channelCount = static_cast<int>(bufferFormat.channelCount),
        .isInterleaved = bufferFormat.isInterleaved != NO,
    };

    auto encoder = std::make_unique<ios_encoder::IOSEncoder>(fileProperties_);
    auto openResult = encoder->open(inputFormat, outputSpec, maxInputBufferLength, filePath_);
    if (openResult.is_err()) {
      return OpenFileResult::Err(openResult.unwrap_err());
    }
    encoder_ = std::move(encoder);

    auto offloaderLambda = [this](WriterData data) {
      taskOffloaderFunction(data);
    };
    offloader_ = std::make_unique<task_offloader::TaskOffloader<
        WriterData,
        FILE_WRITER_SPSC_OVERFLOW_STRATEGY,
        FILE_WRITER_SPSC_WAIT_STRATEGY>>(FILE_WRITER_CHANNEL_CAPACITY, offloaderLambda);

    inputBufferPool_.clear();
    inputBufferPool_.reserve(FILE_WRITER_POOL_SIZE);
    auto bufferCount =
        static_cast<UInt32>(bufferFormat_.isInterleaved ? 1 : bufferFormat_.channelCount);
    auto bytesPerBuffer =
        static_cast<UInt32>(maxInputBufferLength * bufferFormat_.streamDescription->mBytesPerFrame);
    inputBufferBytesPerBuffer_ = bytesPerBuffer;
    for (size_t i = 0; i < FILE_WRITER_POOL_SIZE; ++i) {
      OwnedAudioBufferListPtr buffer(allocateOwnedAudioBufferList(bufferCount, bytesPerBuffer));
      if (buffer == nullptr) {
        rollbackFailedOpen();
        return OpenFileResult::Err("Failed to preallocate iOS file writer buffers");
      }
      inputBufferPool_.push_back(std::move(buffer));
    }

    freeSlots_ = std::make_unique<FreeList>();
    freeSlots_->seed();

    isFileOpen_.store(true, std::memory_order_release);
    return OpenFileResult::Ok(filePath_);
  }
}

void IOSFileWriter::rollbackFailedOpen()
{
  offloader_.reset();
  freeSlots_.reset();
  inputBufferPool_.clear();
  inputBufferBytesPerBuffer_ = 0;

  if (encoder_ != nullptr) {
    encoder_->close();
    encoder_.reset();
  }
  bufferFormat_ = nil;

  framesWritten_.store(0, std::memory_order_release);
  isFileOpen_.store(false, std::memory_order_release);
}

/// @brief Closes the currently open audio file and finalizes writing.
/// This method should be called from the JS thread only.
CloseFileResult IOSFileWriter::closeFile()
{
  @autoreleasepool {
    if (!isFileOpen() || encoder_ == nullptr) {
      return CloseFileResult::Err("file is not open: " + filePath_);
    }

    isFileOpen_.store(false, std::memory_order_release);

    // Drain the worker queue before finalizing the encoder below.
    offloader_.reset();
    freeSlots_.reset();
    inputBufferPool_.clear();
    inputBufferBytesPerBuffer_ = 0;

    auto closeResult = encoder_->close();
    encoder_.reset();
    bufferFormat_ = nil;
    framesWritten_.store(0, std::memory_order_release);

    if (closeResult.is_err()) {
      return CloseFileResult::Err(closeResult.unwrap_err());
    }
    return CloseFileResult::Ok(closeResult.unwrap());
  }
}

/// @brief Copies audio data into an owned buffer and hands it to the worker
/// thread. This method is called on the audio thread.
void IOSFileWriter::writeAudioData(const AudioBufferList *audioBufferList, int numFrames)
{
  if (!isFileOpen() || encoder_ == nullptr) {
    return;
  }
  if (offloader_ == nullptr || freeSlots_ == nullptr || inputBufferPool_.empty() ||
      inputBufferBytesPerBuffer_ == 0) {
    return;
  }

  auto slot = freeSlots_->tryAcquire();
  if (!slot.has_value()) {
    return;
  }

  // CoreAudio owns `audioBufferList` only for the duration of this synchronous
  // callback. Copy into an owned AudioBufferList before handing off to the
  // worker thread; the consumer in taskOffloaderFunction releases the slot.
  auto *targetBuffer = inputBufferPool_[slot.value()].get();
  if (!copyIntoOwnedAudioBufferList(
          targetBuffer, audioBufferList, static_cast<unsigned int>(inputBufferBytesPerBuffer_))) {
    freeSlots_->release(slot.value());
    return;
  }

  WriterData writerData{.slot = slot.value(), .numFrames = numFrames};
  offloader_->getSender()->send(writerData);
}

void IOSFileWriter::taskOffloaderFunction(WriterData data)
{
  auto [slot, numFrames] = data;
  if (slot == FreeList::kSentinel) {
    return;
  }
  if (slot >= inputBufferPool_.size() || freeSlots_ == nullptr || encoder_ == nullptr) {
    return;
  }
  auto *audioBufferList = inputBufferPool_[slot].get();

  auto result = encoder_->encode(audioBufferList, numFrames);
  freeSlots_->release(slot);

  if (result.is_err()) {
    invokeOnErrorCallback(result.unwrap_err());
    return;
  }

  framesWritten_.fetch_add(numFrames, std::memory_order_acq_rel);
}

double IOSFileWriter::getCurrentDuration() const
{
  double sampleRate = inputSampleRate_ > 0 ? inputSampleRate_ : fileProperties_->sampleRate;
  if (sampleRate <= 0) {
    return 0.0;
  }
  return static_cast<double>(framesWritten_.load(std::memory_order_acquire)) / sampleRate;
}

std::string IOSFileWriter::getFilePath() const
{
  return filePath_;
}

size_t IOSFileWriter::getFileSizeBytes() const
{
  if (encoder_ == nullptr) {
    return 0;
  }
  return encoder_->getFileSizeBytes();
}

} // namespace audioapi
