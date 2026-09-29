#include <audioapi/core/utils/RecordingFileName.h>
#include <audioapi/utils/AudioFileProperties.h>
#include <gtest/gtest.h>
#include <memory>
#include <string>

using namespace audioapi;

// NOLINTBEGIN

namespace {

std::shared_ptr<AudioFileProperties> makeProperties(
    const std::string &fileName,
    size_t rotateIntervalBytes = 0) {
  return std::make_shared<AudioFileProperties>(
      AudioFileProperties::PathConfig{
          .directory = AudioFileProperties::FileDirectory::Cache,
          .subDirectory = "AudioAPI",
          .fileName = fileName,
      },
      AudioFileProperties::StreamConfig{.sampleRate = 48000.0F, .channelCount = 2},
      AudioFileProperties::EncodingConfig{
          .format = AudioFileProperties::FileFormat::WAV,
          .bitRate = 128000,
          .bitDepth = AudioFileProperties::BitDepth::Bit16,
          .flacCompressionLevel = 5,
          .iosAudioQuality = AudioFileProperties::IOSAudioQuality::High,
      },
      AudioFileProperties::WriterConfig{
          .rotateIntervalBytes = rotateIntervalBytes,
          .androidFlushIntervalMs = 0,
      });
}

} // namespace

TEST(RecordingFileNameTest, GeneratedStemCarriesATimestamp) {
  const std::string stem = recording_file_name::sessionStem(makeProperties(""));

  ASSERT_TRUE(stem.starts_with("recording_"));
  // "recording_" + "YYYYmmdd_HHMMSS"
  EXPECT_EQ(stem.size(), std::string("recording_").size() + 15);
}

TEST(RecordingFileNameTest, FileNameIsTakenVerbatim) {
  EXPECT_EQ(recording_file_name::sessionStem(makeProperties("session-42")), "session-42");
}

TEST(RecordingFileNameTest, FileNameIsTakenVerbatimWithRotationToo) {
  EXPECT_EQ(recording_file_name::sessionStem(makeProperties("session-42", 1024)), "session-42");
}

TEST(RecordingFileNameTest, SessionStemIsStableAcrossSegments) {
  const std::string stem = recording_file_name::sessionStem(makeProperties("", 1024));

  // Rotation must reuse one stem: per-segment timestamps collide within a second.
  EXPECT_EQ(recording_file_name::segmentStem(stem, 1), stem + "_001");
  EXPECT_EQ(recording_file_name::segmentStem(stem, 2), stem + "_002");
}

TEST(RecordingFileNameTest, SegmentIndexIsZeroPaddedToThreeDigits) {
  EXPECT_EQ(recording_file_name::segmentStem("session", 1), "session_001");
  EXPECT_EQ(recording_file_name::segmentStem("session", 42), "session_042");
  EXPECT_EQ(recording_file_name::segmentStem("session", 999), "session_999");
}

TEST(RecordingFileNameTest, SegmentIndexKeepsCountingPastThePaddedWidth) {
  // Past 999 the names stay unique and only their lexicographic order suffers.
  EXPECT_EQ(recording_file_name::segmentStem("session", 1000), "session_1000");
  EXPECT_EQ(recording_file_name::segmentStem("session", 123456), "session_123456");
}

TEST(RecordingFileNameTest, GeneratedNameNeedsNoValidation) {
  EXPECT_TRUE(recording_file_name::validateFileName("").is_ok());
}

TEST(RecordingFileNameTest, PlainFileNameValidates) {
  EXPECT_TRUE(recording_file_name::validateFileName("session-42").is_ok());
}

TEST(RecordingFileNameTest, FileNameCannotEscapeTheRecordingDirectory) {
  EXPECT_TRUE(recording_file_name::validateFileName("../../etc/passwd").is_err());
  EXPECT_TRUE(recording_file_name::validateFileName("nested/name").is_err());
  EXPECT_TRUE(recording_file_name::validateFileName("nested\\name").is_err());
}

TEST(RecordingFileNameTest, FileNameCannotCarryAnExtension) {
  EXPECT_TRUE(recording_file_name::validateFileName("session-42.wav").is_err());
}

TEST(RecordingFileNameTest, OverlyLongFileNameIsRejected) {
  EXPECT_TRUE(recording_file_name::validateFileName(std::string(129, 'a')).is_err());
  EXPECT_TRUE(recording_file_name::validateFileName(std::string(128, 'a')).is_ok());
}

// NOLINTEND
