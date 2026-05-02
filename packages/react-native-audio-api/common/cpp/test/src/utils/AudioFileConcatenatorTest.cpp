#include <audioapi/core/utils/AudioFileConcatenator.h>
#include <gtest/gtest.h>

#include <string>
#include <vector>

using namespace audioapi;

// NOLINTBEGIN

TEST(AudioFileConcatenatorTest, NormalizesFileUrls) {
  EXPECT_EQ(
      AudioFileConcatenator::normalizeFilePath("file:///tmp/audio%20segment.m4a"),
      "/tmp/audio segment.m4a");
}

TEST(AudioFileConcatenatorTest, KeepsFilesystemPaths) {
  EXPECT_EQ(
      AudioFileConcatenator::normalizeFilePath("/tmp/audio%20segment.m4a"),
      "/tmp/audio segment.m4a");
}

TEST(AudioFileConcatenatorTest, RejectsEmptyInputList) {
  auto result = AudioFileConcatenator::concatAudioFiles({}, "/tmp/output.m4a");

  EXPECT_TRUE(result.is_err());
  EXPECT_EQ(result.unwrap_err(), "concatAudioFiles requires at least one input path.");
}

TEST(AudioFileConcatenatorTest, RejectsEmptyOutputPath) {
  auto result = AudioFileConcatenator::concatAudioFiles({"/tmp/input.m4a"}, "");

  EXPECT_TRUE(result.is_err());
  EXPECT_EQ(result.unwrap_err(), "concatAudioFiles requires an output path.");
}

TEST(AudioFileConcatenatorTest, RejectsEmptyFileUrlOutputPath) {
  auto result = AudioFileConcatenator::concatAudioFiles({"/tmp/input.m4a"}, "file://");

  EXPECT_TRUE(result.is_err());
  EXPECT_EQ(result.unwrap_err(), "concatAudioFiles requires an output path.");
}

TEST(AudioFileConcatenatorTest, RejectsEmptyFileUrlInputPath) {
  auto result = AudioFileConcatenator::concatAudioFiles({"file://"}, "/tmp/output.m4a");

  EXPECT_TRUE(result.is_err());
  EXPECT_EQ(result.unwrap_err(), "concatAudioFiles input path at index 0 is empty.");
}

TEST(AudioFileConcatenatorTest, RejectsNonFileInputProtocol) {
  auto result =
      AudioFileConcatenator::concatAudioFiles({"http://example.com/input.m4a"}, "/tmp/output.m4a");

  EXPECT_TRUE(result.is_err());
  EXPECT_EQ(
      result.unwrap_err(),
      "concatAudioFiles input path at index 0 must be a local file path or file:// URL.");
}

TEST(AudioFileConcatenatorTest, RejectsNonFileOutputProtocol) {
  auto result = AudioFileConcatenator::concatAudioFiles({"/tmp/input.m4a"}, "pipe:output.m4a");

  EXPECT_TRUE(result.is_err());
  EXPECT_EQ(
      result.unwrap_err(),
      "concatAudioFiles output path must be a local file path or file:// URL.");
}

TEST(AudioFileConcatenatorTest, ReturnsDisabledErrorWhenFFmpegIsUnavailable) {
  auto result = AudioFileConcatenator::concatAudioFiles({"/tmp/input.m4a"}, "/tmp/output.m4a");

  EXPECT_TRUE(result.is_err());
  EXPECT_EQ(result.unwrap_err(), "FFmpeg is disabled, cannot concatenate audio files.");
}

// NOLINTEND
