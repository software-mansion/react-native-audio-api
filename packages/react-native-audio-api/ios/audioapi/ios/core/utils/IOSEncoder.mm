#import <AVFoundation/AVFoundation.h>
#import <Foundation/Foundation.h>

#include <audioapi/encoding/EncoderOutputSpec.h>
#include <audioapi/ios/core/utils/IOSEncoder.h>
#include <audioapi/ios/core/utils/IOSEncoderSettings.h>
#include <audioapi/utils/AudioFileProperties.h>
#include <audioapi/utils/UnitConversion.h>

#include <algorithm>
#include <cstddef>
#include <limits>
#include <string>
#include <vector>

#include <audioapi/core/utils/Constants.h>

namespace audioapi::ios::encoder {

struct IOSEncoderState {
  NSURL *fileURL = nil;
  AVAudioFile *audioFile = nil;
  AVAudioFormat *inputFormat = nil;
  AVAudioConverter *converter = nil;
  AVAudioPCMBuffer *converterOutputBuffer = nil;
  int inputChannelCount = 0;
  size_t maxInputFrames = 0;
  /// Backing store for an AudioBufferList with one buffer per input channel. encode() points
  /// it at the caller's planar frames and wraps it in a no-copy AVAudioPCMBuffer.
  std::vector<uint8_t> inputBufferListStorage;

  AudioBufferList *inputBufferList()
  {
    return reinterpret_cast<AudioBufferList *>(inputBufferListStorage.data());
  }
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
    const EncoderSettings &encoderSettings,
    const EncoderOutputSpec &outputSpec)
{
  AudioFormatID formatId = audioFormatIdForCodec(outputSpec.codec);
  NSMutableDictionary *settings = [NSMutableDictionary dictionary];

  settings[AVFormatIDKey] = @(formatId);
  settings[AVSampleRateKey] = @(encoderSettings.stream.sampleRate);
  settings[AVNumberOfChannelsKey] = @(encoderSettings.stream.channelCount);
  settings[AVEncoderAudioQualityKey] = @(getQuality(encoderSettings.encoding));

  if (formatId == kAudioFormatMPEG4AAC && encoderSettings.encoding.bitRate > 0) {
    settings[AVEncoderBitRateKey] = @(encoderSettings.encoding.bitRate);
  }

  if (formatId == kAudioFormatLinearPCM) {
    NSInteger bitDepth = getBitDepth(encoderSettings.encoding);
    settings[AVLinearPCMBitDepthKey] = @(bitDepth);
    settings[AVLinearPCMIsFloatKey] = @(bitDepth == 32);
    settings[AVLinearPCMIsBigEndianKey] = @(NO);
    settings[AVLinearPCMIsNonInterleaved] = @(NO);
  }

  if (formatId == kAudioFormatFLAC) {
    settings[@"FLACCompressionLevel"] = @(getFlacCompressionLevel(encoderSettings.encoding));
  }

  return settings;
}

IOSEncoder::IOSEncoder(const EncoderSettings &settings)
    : AudioEncoder(settings), impl_(std::make_unique<IOSEncoderState>())
{
}

IOSEncoder::~IOSEncoder()
{
  @autoreleasepool {
    impl_->audioFile = nil;
    impl_->converter = nil;
    impl_->converterOutputBuffer = nil;
    impl_->inputFormat = nil;
    impl_->fileURL = nil;
  }
}

OpenEncoderResult IOSEncoder::open(
    const StreamFormat &inputFormat,
    const EncoderOutputSpec &outputSpec,
    const std::string &filePath)
{
  @autoreleasepool {
    if (isOpen()) {
      return Err("Encoder already open");
    }
    if (inputFormat.sampleRate <= 0 || inputFormat.channelCount <= 0) {
      return Err("Invalid input format: sampleRate and channelCount must be greater than 0");
    }
    if (settings_.stream.sampleRate <= 0 || settings_.stream.channelCount <= 0) {
      return Err("Invalid encoder settings: sampleRate and channelCount must be greater than 0");
    }

    outputSpec_ = outputSpec;
    filePath_ = filePath;
    resetFramesEncoded();

    impl_->fileURL = [NSURL fileURLWithPath:[NSString stringWithUTF8String:filePath.c_str()]];

    // encode() receives planar float32, so the processing format is planar too: the frames are
    // handed over without a copy
    NSError *error = nil;
    NSDictionary *settings = buildFileSettings(settings_, outputSpec);
    impl_->audioFile = [[AVAudioFile alloc] initForWriting:impl_->fileURL
                                                  settings:settings
                                              commonFormat:AVAudioPCMFormatFloat32
                                               interleaved:NO
                                                     error:&error];
    if (error != nil || impl_->audioFile == nil) {
      return Err(
          std::string("Error creating audio file for writing: ") +
          (error != nil ? [[error debugDescription] UTF8String] : "unknown"));
    }

    auto pipelineResult = prepareConversionPipeline(inputFormat);
    if (pipelineResult.is_err()) {
      releaseConversionPipeline();
      impl_->audioFile = nil;
      return Err(pipelineResult.unwrap_err());
    }

    markOpen();
    return Ok(filePath_);
  }
}

OpenEncoderResult IOSEncoder::reprepareInput(const StreamFormat &inputFormat)
{
  @autoreleasepool {
    if (!isOpen() || impl_->audioFile == nil) {
      return Err("Encoder is not open");
    }
    if (inputFormat.sampleRate <= 0 || inputFormat.channelCount <= 0) {
      return Err("Invalid input format: sampleRate and channelCount must be greater than 0");
    }

    // The file's settings come from the encoder settings, not the input, so it stays open and the recording
    // is not split; only the converter, which is built for the input, is rebuilt.
    releaseConversionPipeline();

    auto pipelineResult = prepareConversionPipeline(inputFormat);
    if (pipelineResult.is_err()) {
      releaseConversionPipeline();
      return Err(pipelineResult.unwrap_err());
    }

    return Ok(filePath_);
  }
}

Result<NoneType, std::string> IOSEncoder::prepareConversionPipeline(const StreamFormat &inputFormat)
{
  @autoreleasepool {
    if (inputFormat.channelCount > MAX_CHANNEL_COUNT) {
      return Err("Channel count exceeds MAX_CHANNEL_COUNT");
    }
    inputFormat_ = inputFormat;
    impl_->inputChannelCount = inputFormat.channelCount;
    impl_->maxInputFrames = inputFormat.maxFramesPerBuffer;
    impl_->inputBufferListStorage.assign(
        offsetof(AudioBufferList, mBuffers) +
            static_cast<size_t>(inputFormat.channelCount) * sizeof(::AudioBuffer),
        0);

    impl_->inputFormat =
        [[AVAudioFormat alloc] initWithCommonFormat:AVAudioPCMFormatFloat32
                                         sampleRate:inputFormat.sampleRate
                                           channels:(AVAudioChannelCount)inputFormat.channelCount
                                        interleaved:NO];
    if (impl_->inputFormat == nil) {
      return Err("Failed to build input AVAudioFormat");
    }

    impl_->converter =
        [[AVAudioConverter alloc] initFromFormat:impl_->inputFormat
                                        toFormat:[impl_->audioFile processingFormat]];
    if (impl_->converter == nil) {
      return Err("Failed to create AVAudioConverter");
    }
    impl_->converter.sampleRateConverterAlgorithm = AVSampleRateConverterAlgorithm_Normal;
    impl_->converter.sampleRateConverterQuality = AVAudioQualityMax;
    impl_->converter.primeMethod = AVAudioConverterPrimeMethod_None;

    size_t outputCapacity = std::max(
        static_cast<float>(inputFormat.maxFramesPerBuffer),
        settings_.stream.sampleRate / inputFormat.sampleRate * inputFormat.maxFramesPerBuffer);

    impl_->converterOutputBuffer =
        [[AVAudioPCMBuffer alloc] initWithPCMFormat:[impl_->audioFile processingFormat]
                                      frameCapacity:(AVAudioFrameCount)outputCapacity];

    if (impl_->converterOutputBuffer == nil) {
      return Err("Failed to allocate converter buffers");
    }

    return Ok(None);
  }
}

void IOSEncoder::releaseConversionPipeline()
{
  impl_->converter = nil;
  impl_->converterOutputBuffer = nil;
  impl_->inputFormat = nil;
  impl_->inputChannelCount = 0;
  impl_->maxInputFrames = 0;
  impl_->inputBufferListStorage.clear();
}

EncodeResult IOSEncoder::encode(const float *const *channels, int numFrames)
{
  if (!isOpen() || impl_->audioFile == nil) {
    return Err("Encoder is not open");
  }
  if (channels == nullptr || numFrames <= 0) {
    return Err("Invalid encode input");
  }
  if (static_cast<size_t>(numFrames) > impl_->maxInputFrames) {
    return Err("Encode input exceeds the buffer size declared at open()");
  }

  @autoreleasepool {
    NSError *error = nil;

    // Wrap the caller's planar frames in place: the buffer list points at them for the duration
    // of this call and AVAudioPCMBuffer reads through it without copying.
    AudioBufferList *bufferList = impl_->inputBufferList();
    bufferList->mNumberBuffers = static_cast<UInt32>(impl_->inputChannelCount);
    for (int channel = 0; channel < impl_->inputChannelCount; ++channel) {
      bufferList->mBuffers[channel].mNumberChannels = 1;
      bufferList->mBuffers[channel].mDataByteSize =
          static_cast<UInt32>(static_cast<size_t>(numFrames) * sizeof(float));
      // AudioBufferList carries void*; nothing below writes through it.
      bufferList->mBuffers[channel].mData =
          const_cast<float *>(channels[channel]); // NOLINT(cppcoreguidelines-pro-type-const-cast)
    }
    AVAudioPCMBuffer *inputBuffer = [[AVAudioPCMBuffer alloc] initWithPCMFormat:impl_->inputFormat
                                                               bufferListNoCopy:bufferList
                                                                    deallocator:nil];
    if (inputBuffer == nil) {
      return Err("Failed to wrap encode input");
    }

    AVAudioFormat *fileFormat = [impl_->audioFile processingFormat];
    const bool formatsMatch = impl_->inputFormat.sampleRate == fileFormat.sampleRate &&
        impl_->inputFormat.channelCount == fileFormat.channelCount &&
        impl_->inputFormat.isInterleaved == fileFormat.isInterleaved;

    if (formatsMatch) {
      [impl_->audioFile writeFromBuffer:inputBuffer error:&error];
      if (error != nil) {
        return Err(
            std::string("Error writing audio data to file: ") +
            [[error debugDescription] UTF8String]);
      }
      addEncodedFrames(static_cast<size_t>(numFrames));
      return Ok(static_cast<size_t>(numFrames));
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
      return inputBuffer;
    };

    [impl_->converter convertToBuffer:impl_->converterOutputBuffer
                                error:&error
                   withInputFromBlock:inputBlock];
    if (error != nil) {
      return Err(
          std::string("Error during audio conversion: ") + [[error debugDescription] UTF8String]);
    }

    AVAudioFrameCount producedFrames = impl_->converterOutputBuffer.frameLength;
    if (producedFrames == 0) {
      return Ok(static_cast<size_t>(numFrames));
    }

    [impl_->audioFile writeFromBuffer:impl_->converterOutputBuffer error:&error];
    if (error != nil) {
      return Err(
          std::string("Error writing audio data to file: ") +
          [[error debugDescription] UTF8String]);
    }

    addEncodedFrames(static_cast<size_t>(producedFrames));
    return Ok(static_cast<size_t>(numFrames));
  }
}

CloseEncoderResult IOSEncoder::close()
{
  @autoreleasepool {
    if (!isOpen() || impl_->audioFile == nil) {
      return Err("Encoder is not open");
    }
    markClosed();

    // AVAudioFile finalizes the file on deallocation.
    impl_->audioFile = nil;
    impl_->converter = nil;
    impl_->converterOutputBuffer = nil;
    impl_->inputFormat = nil;
    impl_->inputBufferListStorage.clear();

    const double durationSeconds = getEncodedDurationSeconds();
    const double fileSizeMB = static_cast<double>(getFileSizeBytes()) / MB_IN_BYTES;

    impl_->fileURL = nil;
    resetFramesEncoded();

    return Ok(std::make_tuple(fileSizeMB, durationSeconds));
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

} // namespace audioapi::ios::encoder
