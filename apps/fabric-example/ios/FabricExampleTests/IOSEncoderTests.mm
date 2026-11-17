#import <AVFoundation/AVFoundation.h>
#import <XCTest/XCTest.h>

#import <audioapi/encoding/AdtsHeader.h>
#import <audioapi/encoding/EncoderCapabilities.h>
#import <audioapi/ios/core/utils/IOSEncoder.h>
#import <audioapi/utils/AudioBuffer.hpp>
#import <audioapi/utils/AudioFileProperties.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>

using namespace audioapi;

/// The file type AVAudioFile writes follows the path extension, so the ADTS format is
/// proven at the byte level: every frame must start with a sync word and its length must
/// land exactly on the next one.
@interface IOSEncoderAdtsTests : XCTestCase
@end

@implementation IOSEncoderAdtsTests

- (void)testAacExtensionWritesSelfDelimitingAdtsFrames {
  NSString *path = [NSTemporaryDirectory()
      stringByAppendingPathComponent:@"ios-encoder-adts-test.aac"];
  [[NSFileManager defaultManager] removeItemAtPath:path error:nil];

  const AudioLayout layout{.sampleRate = 44100, .channelCount = 1};
  ios::encoder::IOSEncoder encoder(EncoderSettings{
      .fileLayout = layout,
      .encoding =
          AudioFileProperties::EncodingConfig{
              .format = AudioFileProperties::FileFormat::ADTS,
              .bitRate = 64000,
              .bitDepth = AudioFileProperties::BitDepth::Bit16,
              .flacCompressionLevel = 0,
              .iosAudioQuality = AudioFileProperties::IOSAudioQuality::High,
          },
  });
  const EncoderOutputSpec spec =
      encoder_capabilities::specForFormat(AudioFileProperties::FileFormat::ADTS);
  XCTAssertEqual(spec.container, AudioContainer::ADTS);

  auto openResult = encoder.open(
      StreamFormat{.layout = layout, .maxFramesPerBuffer = 1024}, spec, path.UTF8String);
  XCTAssertTrue(openResult.is_ok(), @"%s",
                openResult.is_err() ? openResult.unwrap_err().c_str() : "");

  audioapi::AudioBuffer input(1024, 1, layout.sampleRate);
  std::fill_n(input.getChannel(0)->begin(), 1024, 0.25f);
  for (int block = 0; block < 40; ++block) {
    XCTAssertTrue(encoder.encode(input, 1024).is_ok());
  }
  XCTAssertTrue(encoder.close().is_ok());

  NSData *data = [NSData dataWithContentsOfFile:path];
  XCTAssertGreaterThan(data.length, adts::HEADER_SIZE);
  const auto *bytes = static_cast<const uint8_t *>(data.bytes);

  size_t offset = 0;
  size_t frames = 0;
  while (offset + adts::HEADER_SIZE <= data.length) {
    adts::HeaderBytes header{};
    std::memcpy(header.data(), bytes + offset, adts::HEADER_SIZE);
    XCTAssertEqual(header[0], 0xFF, @"frame %zu", frames);
    XCTAssertEqual(header[1] & 0xF6, 0xF0, @"frame %zu", frames);
    // 13-bit frame length straddles bytes 3..5; computed here because nothing in the iOS
    // library references the ADTS helpers, so they are not linked into the test host.
    const size_t frameLength = ((static_cast<size_t>(header[3]) & 0x3U) << 11) |
        (static_cast<size_t>(header[4]) << 3) | (static_cast<size_t>(header[5]) >> 5);
    XCTAssertGreaterThan(frameLength, adts::HEADER_SIZE, @"frame %zu", frames);
    offset += frameLength;
    frames += 1;
  }
  XCTAssertEqual(offset, data.length);
  XCTAssertGreaterThan(frames, 0u);

  [[NSFileManager defaultManager] removeItemAtPath:path error:nil];
}

@end

/// Input and file channel counts that differ are remixed with the Web Audio speaker rules
/// (AudioBuffer::copy), as on Android. Each input channel carries its own constant level into a
/// float WAV at the input's rate, so every file channel must hold exactly the rule's weighted sum.
@interface IOSEncoderChannelMixTests : XCTestCase
@end

static const float kSqrtHalf = std::sqrt(0.5f);
static const float kTolerance = 1e-5f;

@implementation IOSEncoderChannelMixTests

- (std::vector<float>)levelsAfterEncoding:(const std::vector<float> &)inputLevels
                       intoFileChannels:(int)fileChannelCount {
  NSString *path = [NSTemporaryDirectory()
      stringByAppendingPathComponent:@"ios-encoder-channel-mix-test.wav"];
  [[NSFileManager defaultManager] removeItemAtPath:path error:nil];

  constexpr float sampleRate = 48000;
  constexpr int framesPerBlock = 512;
  const int inputChannelCount = static_cast<int>(inputLevels.size());

  ios::encoder::IOSEncoder encoder(EncoderSettings{
      .fileLayout = {.sampleRate = sampleRate, .channelCount = fileChannelCount},
      .encoding =
          AudioFileProperties::EncodingConfig{
              .format = AudioFileProperties::FileFormat::WAV,
              .bitRate = 0,
              .bitDepth = AudioFileProperties::BitDepth::Bit32,
              .flacCompressionLevel = 0,
              .iosAudioQuality = AudioFileProperties::IOSAudioQuality::High,
          },
  });
  auto openResult = encoder.open(
      StreamFormat{
          .layout = {.sampleRate = sampleRate, .channelCount = inputChannelCount},
          .maxFramesPerBuffer = framesPerBlock},
      encoder_capabilities::specForFormat(AudioFileProperties::FileFormat::WAV),
      path.UTF8String);
  XCTAssertTrue(openResult.is_ok(), @"%s",
                openResult.is_err() ? openResult.unwrap_err().c_str() : "");

  audioapi::AudioBuffer input(framesPerBlock, inputChannelCount, sampleRate);
  for (int channel = 0; channel < inputChannelCount; ++channel) {
    std::fill_n(input.getChannel(channel)->begin(), framesPerBlock, inputLevels[channel]);
  }
  for (int block = 0; block < 8; ++block) {
    XCTAssertTrue(encoder.encode(input, framesPerBlock).is_ok());
  }
  XCTAssertTrue(encoder.close().is_ok());

  NSError *error = nil;
  AVAudioFile *file = [[AVAudioFile alloc] initForReading:[NSURL fileURLWithPath:path]
                                             commonFormat:AVAudioPCMFormatFloat32
                                              interleaved:NO
                                                    error:&error];
  XCTAssertNil(error);
  XCTAssertEqual(file.processingFormat.channelCount, static_cast<AVAudioChannelCount>(fileChannelCount));
  AVAudioPCMBuffer *buffer =
      [[AVAudioPCMBuffer alloc] initWithPCMFormat:file.processingFormat
                                    frameCapacity:static_cast<AVAudioFrameCount>(file.length)];
  XCTAssertTrue([file readIntoBuffer:buffer error:&error]);
  XCTAssertGreaterThan(buffer.frameLength, 0u);

  std::vector<float> levels;
  const AVAudioFrameCount middle = buffer.frameLength / 2;
  for (int channel = 0; channel < fileChannelCount; ++channel) {
    levels.push_back(buffer.floatChannelData[channel][middle]);
  }
  [[NSFileManager defaultManager] removeItemAtPath:path error:nil];
  return levels;
}


- (void)testFiveOneToStereoMixesCenterAndSurroundsIntoBothSides {
  const float left = 0.1f, right = 0.2f, center = 0.3f, lfe = 0.4f;
  const float surroundLeft = 0.05f, surroundRight = 0.06f;
  auto levels =
      [self levelsAfterEncoding:std::vector<float>{left, right, center, lfe, surroundLeft, surroundRight} intoFileChannels:2];

  XCTAssertEqualWithAccuracy(levels[0], left + kSqrtHalf * (center + surroundLeft), kTolerance);
  XCTAssertEqualWithAccuracy(levels[1], right + kSqrtHalf * (center + surroundRight), kTolerance);
}

- (void)testFiveOneToMonoIncludesEveryChannelButLfe {
  const float left = 0.1f, right = 0.2f, center = 0.3f, lfe = 0.4f;
  const float surroundLeft = 0.05f, surroundRight = 0.06f;
  auto levels =
      [self levelsAfterEncoding:std::vector<float>{left, right, center, lfe, surroundLeft, surroundRight} intoFileChannels:1];

  XCTAssertEqualWithAccuracy(
      levels[0],
      kSqrtHalf * (left + right) + center + 0.5f * (surroundLeft + surroundRight),
      kTolerance);
}

- (void)testQuadToStereoAveragesFrontAndSurroundPerSide {
  const float left = 0.1f, right = 0.2f, surroundLeft = 0.3f, surroundRight = 0.4f;
  auto levels = [self levelsAfterEncoding:std::vector<float>{left, right, surroundLeft, surroundRight} intoFileChannels:2];

  XCTAssertEqualWithAccuracy(levels[0], 0.5f * (left + surroundLeft), kTolerance);
  XCTAssertEqualWithAccuracy(levels[1], 0.5f * (right + surroundRight), kTolerance);
}

- (void)testStereoToMonoAveragesBothSides {
  auto levels = [self levelsAfterEncoding:std::vector<float>{0.2f, 0.6f} intoFileChannels:1];

  XCTAssertEqualWithAccuracy(levels[0], 0.4f, kTolerance);
}

- (void)testCountWithoutSpeakerRuleKeepsLeadingChannelsInOrder {
  std::vector<float> inputLevels;
  for (int channel = 0; channel < 16; ++channel) {
    inputLevels.push_back(0.01f * static_cast<float>(channel + 1));
  }
  auto levels = [self levelsAfterEncoding:inputLevels intoFileChannels:2];

  XCTAssertEqualWithAccuracy(levels[0], 0.01f, kTolerance);
  XCTAssertEqualWithAccuracy(levels[1], 0.02f, kTolerance);
}

- (void)testMonoToStereoCopiesIntoBothSides {
  auto levels = [self levelsAfterEncoding:std::vector<float>{0.3f} intoFileChannels:2];

  XCTAssertEqualWithAccuracy(levels[0], 0.3f, kTolerance);
  XCTAssertEqualWithAccuracy(levels[1], 0.3f, kTolerance);
}

@end
