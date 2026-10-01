#import <AVFoundation/AVFoundation.h>
#import <Foundation/Foundation.h>

#include <audioapi/encoding/EncoderOutputSpec.h>
#include <audioapi/ios/core/utils/IOSEncoder.h>
#include <audioapi/utils/AudioFileProperties.h>
#include <audioapi/utils/UnitConversion.h>

#include <algorithm>
#include <cstddef>
#include <limits>
#include <string>
#include <vector>

#include <audioapi/core/utils/Constants.h>

// NOLINTBEGIN
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

static AVAudioQuality avAudioQualityFor(AudioFileProperties::IOSAudioQuality quality)
{
  switch (quality) {
    case AudioFileProperties::IOSAudioQuality::Min:
      return AVAudioQualityMin;
    case AudioFileProperties::IOSAudioQuality::Low:
      return AVAudioQualityLow;
    case AudioFileProperties::IOSAudioQuality::High:
      return AVAudioQualityHigh;
    case AudioFileProperties::IOSAudioQuality::Max:
      return AVAudioQualityMax;
    case AudioFileProperties::IOSAudioQuality::Medium:
    default:
      return AVAudioQualityMedium;
  }
}

static NSInteger bitsForBitDepth(AudioFileProperties::BitDepth bitDepth)
{
  switch (bitDepth) {
    case AudioFileProperties::BitDepth::Bit16:
      return 16;
    case AudioFileProperties::BitDepth::Bit24:
      return 24;
    case AudioFileProperties::BitDepth::Bit32:
    default:
      return 32;
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
  settings[AVEncoderAudioQualityKey] =
      @(avAudioQualityFor(encoderSettings.encoding.iosAudioQuality));

  if (formatId == kAudioFormatMPEG4AAC && encoderSettings.encoding.bitRate > 0) {
    settings[AVEncoderBitRateKey] = @(encoderSettings.encoding.bitRate);
  }

  if (formatId == kAudioFormatLinearPCM) {
    NSInteger bitDepth = bitsForBitDepth(encoderSettings.encoding.bitDepth);
    settings[AVLinearPCMBitDepthKey] = @(bitDepth);
    settings[AVLinearPCMIsFloatKey] = @(bitDepth == 32);
    settings[AVLinearPCMIsBigEndianKey] = @(NO);
    settings[AVLinearPCMIsNonInterleaved] = @(NO);
  }

  if (formatId == kAudioFormatFLAC) {
    settings[@"FLACCompressionLevel"] = @(encoderSettings.encoding.flacCompressionLevel);
  }

  return settings;
}

IOSEncoder::IOSEncoder(const EncoderSettings &settings)
    : AudioEncoder(settings), state_(std::make_unique<IOSEncoderState>())
{
}

IOSEncoder::~IOSEncoder()
{
  @autoreleasepool {
    state_->audioFile = nil;
    state_->converter = nil;
    state_->converterOutputBuffer = nil;
    state_->inputFormat = nil;
    state_->fileURL = nil;
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
    if (inputFormat.layout.sampleRate <= 0 || inputFormat.layout.channelCount <= 0) {
      return Err("Invalid input format: sampleRate and channelCount must be greater than 0");
    }
    if (settings_.stream.sampleRate <= 0 || settings_.stream.channelCount <= 0) {
      return Err("Invalid encoder settings: sampleRate and channelCount must be greater than 0");
    }

    outputSpec_ = outputSpec;
    filePath_ = filePath;
    resetFramesEncoded();

    state_->fileURL = [NSURL fileURLWithPath:[NSString stringWithUTF8String:filePath.c_str()]];

    // encode() receives planar float32, so the processing format is planar too: the frames are
    // handed over without a copy
    NSError *error = nil;
    NSDictionary *settings = buildFileSettings(settings_, outputSpec);
    state_->audioFile = [[AVAudioFile alloc] initForWriting:state_->fileURL
                                                   settings:settings
                                               commonFormat:AVAudioPCMFormatFloat32
                                                interleaved:NO
                                                      error:&error];
    if (error != nil || state_->audioFile == nil) {
      return Err(
          std::string("Error creating audio file for writing: ") +
          (error != nil ? [[error debugDescription] UTF8String] : "unknown"));
    }

    auto pipelineResult = prepareConversionPipeline(inputFormat);
    if (pipelineResult.is_err()) {
      releaseConversionPipeline();
      state_->audioFile = nil;
      return Err(pipelineResult.unwrap_err());
    }

    markOpen();
    return Ok(filePath_);
  }
}

OpenEncoderResult IOSEncoder::reprepareInput(const StreamFormat &inputFormat)
{
  @autoreleasepool {
    if (!isOpen() || state_->audioFile == nil) {
      return Err("Encoder is not open");
    }
    if (inputFormat.layout.sampleRate <= 0 || inputFormat.layout.channelCount <= 0) {
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
    const AudioLayout &inputLayout = inputFormat.layout;
    if (inputLayout.channelCount > MAX_CHANNEL_COUNT) {
      return Err("Channel count exceeds MAX_CHANNEL_COUNT");
    }
    inputFormat_ = inputFormat;
    state_->inputChannelCount = inputLayout.channelCount;
    state_->maxInputFrames = inputFormat.maxFramesPerBuffer;
    state_->inputBufferListStorage.assign(
        offsetof(AudioBufferList, mBuffers) +
            static_cast<size_t>(inputLayout.channelCount) * sizeof(::AudioBuffer),
        0);

    state_->inputFormat =
        [[AVAudioFormat alloc] initWithCommonFormat:AVAudioPCMFormatFloat32
                                         sampleRate:inputLayout.sampleRate
                                           channels:(AVAudioChannelCount)inputLayout.channelCount
                                        interleaved:NO];
    if (state_->inputFormat == nil) {
      return Err("Failed to build input AVAudioFormat");
    }

    state_->converter =
        [[AVAudioConverter alloc] initFromFormat:state_->inputFormat
                                        toFormat:[state_->audioFile processingFormat]];
    if (state_->converter == nil) {
      return Err("Failed to create AVAudioConverter");
    }
    state_->converter.sampleRateConverterAlgorithm = AVSampleRateConverterAlgorithm_Normal;
    state_->converter.sampleRateConverterQuality = AVAudioQualityMax;
    state_->converter.primeMethod = AVAudioConverterPrimeMethod_None;

    size_t outputCapacity = std::max(
        static_cast<float>(inputFormat.maxFramesPerBuffer),
        settings_.stream.sampleRate / inputLayout.sampleRate * inputFormat.maxFramesPerBuffer);

    state_->converterOutputBuffer =
        [[AVAudioPCMBuffer alloc] initWithPCMFormat:[state_->audioFile processingFormat]
                                      frameCapacity:(AVAudioFrameCount)outputCapacity];

    if (state_->converterOutputBuffer == nil) {
      return Err("Failed to allocate converter buffers");
    }

    return Ok(None);
  }
}

void IOSEncoder::releaseConversionPipeline()
{
  state_->converter = nil;
  state_->converterOutputBuffer = nil;
  state_->inputFormat = nil;
  state_->inputChannelCount = 0;
  state_->maxInputFrames = 0;
  state_->inputBufferListStorage.clear();
}

EncodeResult IOSEncoder::encode(const float *const *channels, int numFrames)
{
  if (!isOpen() || state_->audioFile == nil) {
    return Err("Encoder is not open");
  }
  if (channels == nullptr || numFrames <= 0) {
    return Err("Invalid encode input");
  }
  if (static_cast<size_t>(numFrames) > state_->maxInputFrames) {
    return Err("Encode input exceeds the buffer size declared at open()");
  }

  @autoreleasepool {
    NSError *error = nil;

    // Wrap the caller's planar frames in place: the buffer list points at them for the duration
    // of this call and AVAudioPCMBuffer reads through it without copying.
    AudioBufferList *bufferList = state_->inputBufferList();
    bufferList->mNumberBuffers = static_cast<UInt32>(state_->inputChannelCount);
    for (int channel = 0; channel < state_->inputChannelCount; ++channel) {
      bufferList->mBuffers[channel].mNumberChannels = 1;
      bufferList->mBuffers[channel].mDataByteSize =
          static_cast<UInt32>(static_cast<size_t>(numFrames) * sizeof(float));
      // AudioBufferList carries void*; nothing below writes through it.
      bufferList->mBuffers[channel].mData =
          const_cast<float *>(channels[channel]); // NOLINT(cppcoreguidelines-pro-type-const-cast)
    }
    AVAudioPCMBuffer *inputBuffer = [[AVAudioPCMBuffer alloc] initWithPCMFormat:state_->inputFormat
                                                               bufferListNoCopy:bufferList
                                                                    deallocator:nil];
    if (inputBuffer == nil) {
      return Err("Failed to wrap encode input");
    }

    AVAudioFormat *fileFormat = [state_->audioFile processingFormat];
    const bool formatsMatch = state_->inputFormat.sampleRate == fileFormat.sampleRate &&
        state_->inputFormat.channelCount == fileFormat.channelCount &&
        state_->inputFormat.isInterleaved == fileFormat.isInterleaved;

    if (formatsMatch) {
      [state_->audioFile writeFromBuffer:inputBuffer error:&error];
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

    [state_->converter convertToBuffer:state_->converterOutputBuffer
                                 error:&error
                    withInputFromBlock:inputBlock];
    if (error != nil) {
      return Err(
          std::string("Error during audio conversion: ") + [[error debugDescription] UTF8String]);
    }

    AVAudioFrameCount producedFrames = state_->converterOutputBuffer.frameLength;
    if (producedFrames == 0) {
      return Ok(static_cast<size_t>(numFrames));
    }

    [state_->audioFile writeFromBuffer:state_->converterOutputBuffer error:&error];
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
    if (!isOpen() || state_->audioFile == nil) {
      return Err("Encoder is not open");
    }
    markClosed();

    // AVAudioFile finalizes the file on deallocation.
    state_->audioFile = nil;
    state_->converter = nil;
    state_->converterOutputBuffer = nil;
    state_->inputFormat = nil;
    state_->inputBufferListStorage.clear();

    const double durationSeconds = getEncodedDurationSeconds();
    const double fileSizeMB = static_cast<double>(getFileSizeBytes()) / MB_IN_BYTES;

    state_->fileURL = nil;
    resetFramesEncoded();

    return Ok(std::make_tuple(fileSizeMB, durationSeconds));
  }
}

size_t IOSEncoder::getFileSizeBytes() const
{
  @autoreleasepool {
    if (state_->fileURL == nil) {
      return 0;
    }
    NSError *error = nil;
    NSDictionary *attrs =
        [[NSFileManager defaultManager] attributesOfItemAtPath:[state_->fileURL path] error:&error];
    if (error != nil || attrs == nil) {
      return 0;
    }
    return static_cast<size_t>([attrs fileSize]);
  }
}

} // namespace audioapi::ios::encoder
// NOLINTEND
