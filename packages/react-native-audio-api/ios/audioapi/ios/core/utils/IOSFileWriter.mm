#import <AVFoundation/AVFoundation.h>
#import <Foundation/Foundation.h>

#include <audioapi/events/AudioEventHandlerRegistry.h>
#include <audioapi/ios/core/utils/FileOptions.h>
#include <audioapi/ios/core/utils/IOSFileWriter.h>
#include <audioapi/ios/core/utils/OwnedAudioBufferList.h>
#include <audioapi/utils/AudioFileProperties.h>
#include <audioapi/utils/Result.hpp>
#include <audioapi/utils/UnitConversion.h>
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

    fileURL_ = nil;
    audioFile_ = nil;
    converter_ = nil;
    bufferFormat_ = nil;
  }
}

/// @brief Opens an audio file for writing with the specified buffer format and maximum input buffer length.
/// This method initializes the AVAudioFile and AVAudioConverter for audio data writing and conversion.
/// This method should be called from the JS thread only.
/// @param bufferFormat The audio format of the input buffer.
/// @param maxInputBufferLength The maximum length of the input buffer in frames.
/// @returns An OpenFileResult indicating success with the file path or an error message.
OpenFileResult IOSFileWriter::openFile(
    AVAudioFormat *bufferFormat,
    size_t maxInputBufferLength,
    const std::string &fileNameOverride)
{
  @autoreleasepool {
    if (audioFile_ != nil) {
      return OpenFileResult::Err("file already open");
    }

    framesWritten_.store(0, std::memory_order_release);
    bufferFormat_ = bufferFormat;

    NSError *error = nil;
    NSDictionary *settings = ios::fileoptions::getFileSettings(fileProperties_);
    fileURL_ = ios::fileoptions::getFileURL(fileProperties_, fileNameOverride);

    if (fileProperties_->sampleRate == 0 || fileProperties_->channelCount == 0) {
      return OpenFileResult::Err(
          "Invalid file properties: sampleRate and channelCount must be greater than 0");
    }

    if (bufferFormat.sampleRate == 0 || bufferFormat.channelCount == 0) {
      return OpenFileResult::Err(
          "Invalid input format: sampleRate and channelCount must be greater than 0");
    }

    audioFile_ = [[AVAudioFile alloc] initForWriting:fileURL_
                                            settings:settings
                                        commonFormat:AVAudioPCMFormatFloat32
                                         interleaved:bufferFormat.interleaved
                                               error:&error];

    if (error != nil) {
      return OpenFileResult::Err(
          std::string("Error creating audio file for writing: ") +
          [[error debugDescription] UTF8String]);
    }

    converter_ = [[AVAudioConverter alloc] initFromFormat:bufferFormat
                                                 toFormat:[audioFile_ processingFormat]];
    converter_.sampleRateConverterAlgorithm = AVSampleRateConverterAlgorithm_Normal;
    converter_.sampleRateConverterQuality = AVAudioQualityMax;
    converter_.primeMethod = AVAudioConverterPrimeMethod_None;

    converterInputBufferSize_ = maxInputBufferLength;
    converterOutputBufferSize_ = std::max(
        (double)maxInputBufferLength,
        fileProperties_->sampleRate / bufferFormat.sampleRate * maxInputBufferLength);

    converterInputBuffer_ =
        [[AVAudioPCMBuffer alloc] initWithPCMFormat:bufferFormat
                                      frameCapacity:(AVAudioFrameCount)maxInputBufferLength];
    converterOutputBuffer_ =
        [[AVAudioPCMBuffer alloc] initWithPCMFormat:[audioFile_ processingFormat]
                                      frameCapacity:(AVAudioFrameCount)converterOutputBufferSize_];

    if (converterInputBuffer_ == nil || converterOutputBuffer_ == nil || audioFile_ == nil ||
        converter_ == nil) {
      rollbackFailedOpen();
      return OpenFileResult::Err("Error creating converter buffers");
    }

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
    auto bytesPerBuffer = static_cast<UInt32>(
        converterInputBufferSize_ * bufferFormat_.streamDescription->mBytesPerFrame);
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
    return OpenFileResult::Ok([[fileURL_ path] UTF8String]);
  }
}

void IOSFileWriter::rollbackFailedOpen()
{
  offloader_.reset();
  freeSlots_.reset();
  inputBufferPool_.clear();
  inputBufferBytesPerBuffer_ = 0;

  audioFile_ = nil;
  converter_ = nil;
  converterInputBuffer_ = nil;
  converterOutputBuffer_ = nil;
  bufferFormat_ = nil;

  if (fileURL_ != nil) {
    [[NSFileManager defaultManager] removeItemAtURL:fileURL_ error:nil];
    fileURL_ = nil;
  }

  framesWritten_.store(0, std::memory_order_release);
  isFileOpen_.store(false, std::memory_order_release);
}

/// @brief Closes the currently open audio file and finalizes writing.
/// This method retrieves the final file duration and size before closing.
/// This method should be called from the JS thread only.
/// @returns A CloseFileResult indicating success with file duration and size or an error message.
CloseFileResult IOSFileWriter::closeFile()
{
  @autoreleasepool {
    NSError *error;
    std::string filePath = [[fileURL_ path] UTF8String];

    if (!isFileOpen() || audioFile_ == nil) {
      return CloseFileResult::Err("file is not open: " + filePath);
    }

    isFileOpen_.store(false, std::memory_order_release);

    offloader_.reset();
    freeSlots_.reset();
    inputBufferPool_.clear();
    inputBufferBytesPerBuffer_ = 0;

    // AVAudioFile automatically finalizes the file when deallocated
    audioFile_ = nil;

    double fileDuration = CMTimeGetSeconds([[AVURLAsset URLAssetWithURL:fileURL_
                                                                options:nil] duration]);
    double fileSizeBytesMb = static_cast<double>([[[NSFileManager defaultManager]
                                 attributesOfItemAtPath:fileURL_.path
                                                  error:&error] fileSize]) /
        MB_IN_BYTES;

    if (error != nil) {
      NSLog(@"⚠️ closeFile: error while retrieving file size");
      fileSizeBytesMb = 0;
    }

    fileURL_ = nil;
    framesWritten_.store(0, std::memory_order_release);

    return CloseFileResult::Ok(std::make_tuple(fileSizeBytesMb, fileDuration));
  }
}

/// @brief Writes audio data to the open audio file, performing format conversion if necessary.
/// This method should be called from the audio thread.
void IOSFileWriter::writeAudioData(const AudioBufferList *audioBufferList, int numFrames)
{
  if (!isFileOpen() || audioFile_ == nil) {
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
  // send() cannot block here: we hold a slot from a pool of FILE_WRITER_POOL_SIZE,
  // and the channel is sized one larger,
  // so the ring always has room while any slot is in flight.
  offloader_->getSender()->send(writerData);
}

void IOSFileWriter::taskOffloaderFunction(WriterData data)
{
  auto [slot, numFrames] = data;
  if (slot == FreeList::kSentinel) {
    return;
  }
  if (slot >= inputBufferPool_.size() || freeSlots_ == nullptr) {
    return;
  }
  auto *audioBufferList = inputBufferPool_[slot].get();
  @autoreleasepool {
    NSError *error = nil;

    for (size_t i = 0; i < bufferFormat_.channelCount; ++i) {
      memcpy(
          converterInputBuffer_.mutableAudioBufferList->mBuffers[i].mData,
          audioBufferList->mBuffers[i].mData,
          audioBufferList->mBuffers[i].mDataByteSize);
    }

    freeSlots_->release(slot);
    converterInputBuffer_.frameLength = numFrames;

    AVAudioFormat *fileFormat = [audioFile_ processingFormat];

    if (bufferFormat_.sampleRate == fileFormat.sampleRate &&
        bufferFormat_.channelCount == fileFormat.channelCount &&
        bufferFormat_.isInterleaved == fileFormat.isInterleaved) {
      [audioFile_ writeFromBuffer:converterInputBuffer_ error:&error];

      if (error != nil) {
        invokeOnErrorCallback(
            std::string("Error writing audio data to file, native error: ") +
            [[error debugDescription] UTF8String]);
        return;
      }

      framesWritten_.fetch_add(numFrames, std::memory_order_acq_rel);
      return;
    }

    __block BOOL handedOff = false;
    AVAudioConverterInputBlock inputBlock = ^AVAudioBuffer *_Nullable(
        AVAudioPacketCount inNumberOfPackets, AVAudioConverterInputStatus *outStatus)
    {
      if (handedOff) {
        *outStatus = AVAudioConverterInputStatus_NoDataNow;
        return nil;
      }

      handedOff = true;
      *outStatus = AVAudioConverterInputStatus_HaveData;
      return converterInputBuffer_;
    };

    [converter_ convertToBuffer:converterOutputBuffer_ error:&error withInputFromBlock:inputBlock];

    if (error != nil) {
      invokeOnErrorCallback(
          std::string("Error during audio conversion, native error: ") +
          [[error debugDescription] UTF8String]);
      return;
    }

    AVAudioFrameCount producedFrames = converterOutputBuffer_.frameLength;
    if (producedFrames == 0) {
      return;
    }

    [audioFile_ writeFromBuffer:converterOutputBuffer_ error:&error];

    if (error != nil) {
      invokeOnErrorCallback(
          std::string("Error writing audio data to file, native error: ") +
          [[error debugDescription] UTF8String]);
      return;
    }

    framesWritten_.fetch_add(producedFrames, std::memory_order_acq_rel);
  }
}

double IOSFileWriter::getCurrentDuration() const
{
  return static_cast<double>(framesWritten_.load(std::memory_order_acquire)) /
      fileProperties_->sampleRate;
}

std::string IOSFileWriter::getFilePath() const
{
  return [[fileURL_ path] UTF8String];
}

size_t IOSFileWriter::getFileSizeBytes() const
{
  @autoreleasepool {
    if (fileURL_ == nil) {
      return 0;
    }
    NSError *error = nil;
    NSDictionary *attrs = [[NSFileManager defaultManager] attributesOfItemAtPath:[fileURL_ path]
                                                                           error:&error];
    if (error != nil || attrs == nil) {
      return 0;
    }
    return static_cast<size_t>([attrs fileSize]);
  }
}

} // namespace audioapi
