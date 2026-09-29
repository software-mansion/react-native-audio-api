#include <audioapi/utils/AudioFileProperties.h>
#include <gtest/gtest.h>

#include <string>

using namespace audioapi;

// NOLINTBEGIN

namespace {

AudioFileProperties validProperties() {
  return AudioFileProperties(
      AudioFileProperties::PathConfig{
          .directory = AudioFileProperties::FileDirectory::Cache,
          .subDirectory = "AudioAPI",
          .fileName = "session",
      },
      AudioFileProperties::StreamConfig{.sampleRate = 48000.0F, .channelCount = 2},
      AudioFileProperties::EncodingConfig{
          .format = AudioFileProperties::FileFormat::FLAC,
          .bitRate = 0,
          .bitDepth = AudioFileProperties::BitDepth::Bit16,
          .flacCompressionLevel = 5,
          .iosAudioQuality = AudioFileProperties::IOSAudioQuality::High,
      },
      AudioFileProperties::WriterConfig{.rotateIntervalBytes = 0, .androidFlushIntervalMs = 0});
}

} // namespace

TEST(AudioFilePropertiesTest, AcceptsUsableProperties) {
  EXPECT_TRUE(validProperties().validate().is_ok());
}

TEST(AudioFilePropertiesTest, RejectsAnUnusableFileName) {
  auto properties = validProperties();
  properties.path.fileName = "../escape";
  EXPECT_TRUE(properties.validate().is_err());
}

TEST(AudioFilePropertiesTest, RejectsANonPositiveSampleRate) {
  auto properties = validProperties();
  properties.stream.sampleRate = 0.0F;
  EXPECT_TRUE(properties.validate().is_err());
}

TEST(AudioFilePropertiesTest, AcceptsOnlyMonoOrStereo) {
  auto properties = validProperties();
  properties.stream.channelCount = 0;
  EXPECT_TRUE(properties.validate().is_err());

  properties.stream.channelCount = 1;
  EXPECT_TRUE(properties.validate().is_ok());

  properties.stream.channelCount = 3;
  EXPECT_TRUE(properties.validate().is_err());
}

TEST(AudioFilePropertiesTest, RejectsAnEnumHoldingNoneOfItsValues) {
  auto properties = validProperties();
  properties.encoding.format = static_cast<AudioFileProperties::FileFormat>(200);
  EXPECT_TRUE(properties.validate().is_err());
}

TEST(AudioFilePropertiesTest, RejectsAFlacLevelOutsideZeroToEight) {
  auto properties = validProperties();
  properties.encoding.flacCompressionLevel = 9;
  EXPECT_TRUE(properties.validate().is_err());

  properties.encoding.flacCompressionLevel = 8;
  EXPECT_TRUE(properties.validate().is_ok());
}

// NOLINTEND
