#import <AVFoundation/AVFoundation.h>
#import <Foundation/Foundation.h>

#include <audioapi/encoding/EncoderOutputSpec.h>
#include <audioapi/ios/core/utils/FileOptions.h>
#include <audioapi/ios/core/utils/IOSEncoding.h>
#include <audioapi/utils/AudioFileProperties.h>
#include <audioapi/utils/UnitConversion.h>

#include <algorithm>
#include <limits>
#include <string>

namespace audioapi::ios_encoder {

struct IOSEncoderState {
  NSURL *fileURL = nil;
  AVAudioFile *audioFile = nil;
  AVAudioFormat *inputFormat = nil;
  AVAudioConverter *converter = nil;
  AVAudioPCMBuffer *converterInputBuffer = nil;
  AVAudioPCMBuffer *converterOutputBuffer = nil;
  int inputChannelCount = 0;
};

static AudioFormatID audioFormatIdForCodec(AudioCodec codec)
{
  switch (codec) {
    case AudioCodec::PCM:
      return kAudioFormatLinearPCM;
    case AudioCodec::AAC:
      return kAudioFormatMPEG4AAC;
    case AudioCodec::ALAC:
      return kAudioFormatAppleLossless;
    case AudioCodec::FLAC:
      return kAudioFormatFLAC;
    case AudioCodec::ULAW:
      return kAudioFormatULaw;
    case AudioCodec::ALAW:
      return kAudioFormatALaw;
    default:
      return kAudioFormatLinearPCM;
  }
}

static NSDictionary *buildFileSettings(
    const std::shared_ptr<AudioFileProperties> &properties,
    const EncoderOutputSpec &outputSpec)
{
  AudioFormatID formatId = audioFormatIdForCodec(outputSpec.codec);
  NSMutableDictionary *settings = [NSMutableDictionary dictionary];

  settings[AVFormatIDKey] = @(formatId);
  settings[AVSampleRateKey] = @(properties->sampleRate);
  settings[AVNumberOfChannelsKey] = @(properties->channelCount);
  settings[AVEncoderAudioQualityKey] = @(ios::fileoptions::getQuality(properties));

  if (formatId == kAudioFormatMPEG4AAC && properties->bitRate > 0) {
    settings[AVEncoderBitRateKey] = @(properties->bitRate);
  }

  if (formatId == kAudioFormatLinearPCM) {
    NSInteger bitDepth = ios::fileoptions::getBitDepth(properties);
    settings[AVLinearPCMBitDepthKey] = @(bitDepth);
    settings[AVLinearPCMIsFloatKey] = @(bitDepth == 32);
    settings[AVLinearPCMIsBigEndianKey] = @(NO);
    settings[AVLinearPCMIsNonInterleaved] = @(NO);
  }

  if (formatId == kAudioFormatFLAC) {
    settings[@"FLACCompressionLevel"] = @(ios::fileoptions::getFlacCompressionLevel(properties));
  }

  return settings;
}

IOSEncoder::IOSEncoder(const std::shared_ptr<AudioFileProperties> &fileProperties)
    : AudioEncoder(fileProperties), impl_(std::make_unique<IOSEncoderState>())
{
}

IOSEncoder::~IOSEncoder()
{
  @autoreleasepool {
    impl_->audioFile = nil;
    impl_->converter = nil;
    impl_->converterInputBuffer = nil;
    impl_->converterOutputBuffer = nil;
    impl_->inputFormat = nil;
    impl_->fileURL = nil;
  }
}

OpenEncoderResult IOSEncoder::open(
    const StreamFormat &inputFormat,
    const EncoderOutputSpec &outputSpec,
    size_t maxBufferSizeInFrames,
    const std::string &filePath)
{
  @autoreleasepool {
    if (isOpen()) {
      return OpenEncoderResult::Err("Encoder already open");
    }
    if (inputFormat.sampleRate <= 0 || inputFormat.channelCount <= 0) {
      return OpenEncoderResult::Err(
          "Invalid input format: sampleRate and channelCount must be greater than 0");
    }
    if (fileProperties_->sampleRate <= 0 || fileProperties_->channelCount <= 0) {
      return OpenEncoderResult::Err(
          "Invalid file properties: sampleRate and channelCount must be greater than 0");
    }

    inputFormat_ = inputFormat;
    outputSpec_ = outputSpec;
    maxBufferSizeInFrames_ = maxBufferSizeInFrames;
    filePath_ = filePath;
    resetFramesEncoded();

    impl_->inputChannelCount = inputFormat.channelCount;
    impl_->fileURL = [NSURL fileURLWithPath:[NSString stringWithUTF8String:filePath.c_str()]];

    impl_->inputFormat =
        [[AVAudioFormat alloc] initWithCommonFormat:AVAudioPCMFormatFloat32
                                         sampleRate:inputFormat.sampleRate
                                           channels:(AVAudioChannelCount)inputFormat.channelCount
                                        interleaved:inputFormat.isInterleaved ? YES : NO];
    if (impl_->inputFormat == nil) {
      return OpenEncoderResult::Err("Failed to build input AVAudioFormat");
    }

    NSError *error = nil;
    NSDictionary *settings = buildFileSettings(fileProperties_, outputSpec);
    impl_->audioFile = [[AVAudioFile alloc] initForWriting:impl_->fileURL
                                                  settings:settings
                                              commonFormat:AVAudioPCMFormatFloat32
                                               interleaved:impl_->inputFormat.interleaved
                                                     error:&error];
    if (error != nil || impl_->audioFile == nil) {
      return OpenEncoderResult::Err(
          std::string("Error creating audio file for writing: ") +
          (error != nil ? [[error debugDescription] UTF8String] : "unknown"));
    }

    impl_->converter =
        [[AVAudioConverter alloc] initFromFormat:impl_->inputFormat
                                        toFormat:[impl_->audioFile processingFormat]];
    if (impl_->converter == nil) {
      impl_->audioFile = nil;
      return OpenEncoderResult::Err("Failed to create AVAudioConverter");
    }
    impl_->converter.sampleRateConverterAlgorithm = AVSampleRateConverterAlgorithm_Normal;
    impl_->converter.sampleRateConverterQuality = AVAudioQualityMax;
    impl_->converter.primeMethod = AVAudioConverterPrimeMethod_None;

    size_t outputCapacity = std::max(
        static_cast<float>(maxBufferSizeInFrames),
        fileProperties_->sampleRate / inputFormat.sampleRate * maxBufferSizeInFrames);

    impl_->converterInputBuffer =
        [[AVAudioPCMBuffer alloc] initWithPCMFormat:impl_->inputFormat
                                      frameCapacity:(AVAudioFrameCount)maxBufferSizeInFrames];
    impl_->converterOutputBuffer =
        [[AVAudioPCMBuffer alloc] initWithPCMFormat:[impl_->audioFile processingFormat]
                                      frameCapacity:(AVAudioFrameCount)outputCapacity];

    if (impl_->converterInputBuffer == nil || impl_->converterOutputBuffer == nil) {
      impl_->audioFile = nil;
      impl_->converter = nil;
      return OpenEncoderResult::Err("Failed to allocate converter buffers");
    }

    markOpen();
    return OpenEncoderResult::Ok(filePath_);
  }
}

EncodeResult IOSEncoder::encode(const void *data, int numFrames)
{
  if (!isOpen() || impl_->audioFile == nil) {
    return EncodeResult::Err("Encoder is not open");
  }
  const auto *audioBufferList = static_cast<const AudioBufferList *>(data);
  if (audioBufferList == nullptr || numFrames <= 0) {
    return EncodeResult::Err("Invalid encode input");
  }

  @autoreleasepool {
    NSError *error = nil;

    UInt32 buffersToCopy = std::min<UInt32>(
        audioBufferList->mNumberBuffers,
        impl_->converterInputBuffer.mutableAudioBufferList->mNumberBuffers);
    for (UInt32 i = 0; i < buffersToCopy; ++i) {
      memcpy(
          impl_->converterInputBuffer.mutableAudioBufferList->mBuffers[i].mData,
          audioBufferList->mBuffers[i].mData,
          audioBufferList->mBuffers[i].mDataByteSize);
    }
    impl_->converterInputBuffer.frameLength = numFrames;

    AVAudioFormat *fileFormat = [impl_->audioFile processingFormat];
    const bool formatsMatch = impl_->inputFormat.sampleRate == fileFormat.sampleRate &&
        impl_->inputFormat.channelCount == fileFormat.channelCount &&
        impl_->inputFormat.isInterleaved == fileFormat.isInterleaved;

    if (formatsMatch) {
      [impl_->audioFile writeFromBuffer:impl_->converterInputBuffer error:&error];
      if (error != nil) {
        return EncodeResult::Err(
            std::string("Error writing audio data to file: ") +
            [[error debugDescription] UTF8String]);
      }
      addEncodedFrames(static_cast<size_t>(numFrames));
      return EncodeResult::Ok(static_cast<size_t>(numFrames));
    }

    __block BOOL handedOff = NO;
    AVAudioConverterInputBlock inputBlock = ^AVAudioBuffer *_Nullable(
        AVAudioPacketCount inNumberOfPackets, AVAudioConverterInputStatus *outStatus)
    {
      if (handedOff) {
        *outStatus = AVAudioConverterInputStatus_NoDataNow;
        return nil;
      }
      handedOff = YES;
      *outStatus = AVAudioConverterInputStatus_HaveData;
      return impl_->converterInputBuffer;
    };

    [impl_->converter convertToBuffer:impl_->converterOutputBuffer
                                error:&error
                   withInputFromBlock:inputBlock];
    if (error != nil) {
      return EncodeResult::Err(
          std::string("Error during audio conversion: ") + [[error debugDescription] UTF8String]);
    }

    AVAudioFrameCount producedFrames = impl_->converterOutputBuffer.frameLength;
    if (producedFrames == 0) {
      return EncodeResult::Ok(static_cast<size_t>(numFrames));
    }

    [impl_->audioFile writeFromBuffer:impl_->converterOutputBuffer error:&error];
    if (error != nil) {
      return EncodeResult::Err(
          std::string("Error writing audio data to file: ") +
          [[error debugDescription] UTF8String]);
    }

    addEncodedFrames(static_cast<size_t>(producedFrames));
    return EncodeResult::Ok(static_cast<size_t>(numFrames));
  }
}

CloseEncoderResult IOSEncoder::close()
{
  @autoreleasepool {
    if (!isOpen() || impl_->audioFile == nil) {
      return CloseEncoderResult::Err("Encoder is not open");
    }
    markClosed();

    NSURL *fileURL = impl_->fileURL;

    // AVAudioFile finalizes the file on deallocation.
    impl_->audioFile = nil;
    impl_->converter = nil;
    impl_->converterInputBuffer = nil;
    impl_->converterOutputBuffer = nil;
    impl_->inputFormat = nil;

    double durationSeconds = CMTimeGetSeconds([[AVURLAsset URLAssetWithURL:fileURL
                                                                   options:nil] duration]);

    NSError *error = nil;
    double fileSizeMB = static_cast<double>([[[NSFileManager defaultManager]
                            attributesOfItemAtPath:[fileURL path]
                                             error:&error] fileSize]) /
        MB_IN_BYTES;
    if (error != nil) {
      fileSizeMB = 0.0;
    }

    impl_->fileURL = nil;
    resetFramesEncoded();

    return CloseEncoderResult::Ok(std::make_tuple(fileSizeMB, durationSeconds));
  }
}

size_t IOSEncoder::getFileSizeBytes() const
{
  @autoreleasepool {
    if (impl_->fileURL == nil) {
      return 0;
    }
    NSError *error = nil;
    NSDictionary *attrs =
        [[NSFileManager defaultManager] attributesOfItemAtPath:[impl_->fileURL path] error:&error];
    if (error != nil || attrs == nil) {
      return 0;
    }
    return static_cast<size_t>([attrs fileSize]);
  }
}

} // namespace audioapi::ios_encoder
