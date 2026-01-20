#import <AVFoundation/AVFoundation.h>
#import <Foundation/Foundation.h>

#include <audioapi/events/AudioEventHandlerRegistry.h>
#include <audioapi/ios/core/utils/FileOptions.h>
#include <audioapi/ios/core/utils/IOSFileWriter.h>
#include <audioapi/utils/AudioFileProperties.h>
#include <audioapi/utils/Result.hpp>
#include <audioapi/utils/UnitConversion.h>

namespace audioapi {
IOSFileWriter::IOSFileWriter(
    const std::shared_ptr<AudioEventHandlerRegistry> &audioEventHandlerRegistry,
    const std::shared_ptr<AudioFileProperties> &fileProperties)
    : AudioFileWriter(audioEventHandlerRegistry, fileProperties)
{
}

IOSFileWriter::~IOSFileWriter()
{
  @autoreleasepool {
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
      audioFile_ = nil;
      converter_ = nil;
      converterInputBuffer_ = nil;
      converterOutputBuffer_ = nil;

      return OpenFileResult::Err("Error creating converter buffers");
    }

    return OpenFileResult::Ok([[fileURL_ path] UTF8String]);
  }
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

    if (audioFile_ == nil) {
      return CloseFileResult::Err("file is not open: " + filePath);
    }

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

    return CloseFileResult::Ok(std::make_tuple(fileDuration, fileSizeBytesMb));
  }
}

/// @brief Writes audio data to the open audio file, performing format conversion if necessary.
/// This method should be called from the audio thread.
/// @param audioBufferList Pointer to the AudioBufferList containing the audio data to write.
/// @param numFrames Number of audio frames in the audioBufferList.
/// @returns True if the write operation was successful, false otherwise.
bool IOSFileWriter::writeAudioData(const AudioBufferList *audioBufferList, int numFrames)
{
  if (audioFile_ == nil) {
    invokeOnErrorCallback("Attempted to write audio data when file is not open");
    return false;
  }

  @autoreleasepool {
    NSError *error = nil;
    AVAudioFormat *fileFormat = [audioFile_ processingFormat];

    if (bufferFormat_.sampleRate == fileFormat.sampleRate &&
        bufferFormat_.channelCount == fileFormat.channelCount &&
        bufferFormat_.isInterleaved == fileFormat.isInterleaved) {
      // We can use the converter input buffer as a "transport" layer to the file
      for (size_t i = 0; i < bufferFormat_.channelCount; ++i) {
        memcpy(
            converterInputBuffer_.mutableAudioBufferList->mBuffers[i].mData,
            audioBufferList->mBuffers[i].mData,
            audioBufferList->mBuffers[i].mDataByteSize);
      }
      converterInputBuffer_.frameLength = numFrames;

      [audioFile_ writeFromBuffer:converterInputBuffer_ error:&error];

      if (error != nil) {
        invokeOnErrorCallback(
            std::string("Error writing audio data to file, native error: ") +
            [[error debugDescription] UTF8String]);
        return false;
      }

      framesWritten_.fetch_add(numFrames, std::memory_order_acq_rel);
      return true;
    }

    for (size_t i = 0; i < bufferFormat_.channelCount; ++i) {
      memcpy(
          converterInputBuffer_.mutableAudioBufferList->mBuffers[i].mData,
          audioBufferList->mBuffers[i].mData,
          audioBufferList->mBuffers[i].mDataByteSize);
    }

    converterInputBuffer_.frameLength = numFrames;

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
    converterOutputBuffer_.frameLength =
        fileProperties_->sampleRate / bufferFormat_.sampleRate * numFrames;

    if (error != nil) {
      invokeOnErrorCallback(
          std::string("Error during audio conversion, native error: ") +
          [[error debugDescription] UTF8String]);
      return false;
    }

    [audioFile_ writeFromBuffer:converterOutputBuffer_ error:&error];

    if (error != nil) {
      invokeOnErrorCallback(
          std::string("Error writing audio data to file, native error: ") +
          [[error debugDescription] UTF8String]);
      return false;
    }

    framesWritten_.fetch_add(numFrames, std::memory_order_acq_rel);
    return true;
  }
}

double IOSFileWriter::getCurrentDuration() const
{
  return static_cast<double>(framesWritten_.load(std::memory_order_acquire)) /
      bufferFormat_.sampleRate;
}

std::string IOSFileWriter::getFilePath() const
{
  return [[fileURL_ path] UTF8String];
}

} // namespace audioapi
