#include <audioapi/core/utils/AudioDecoding.h>
#include <audioapi/libs/miniaudio/miniaudio.h>
#include <gtest/gtest.h>

#include <cmath>
#include <cstdio>
#include <fstream>
#include <limits>
#include <string>
#include <vector>

using namespace audioapi;

// NOLINTBEGIN

namespace {

constexpr ma_uint32 sampleRate = 48000;
constexpr ma_uint32 channelCount = 1;
constexpr float expectedWavDurationSeconds = 0.5F;

std::string testFilePath(const std::string &name) {
  return ::testing::TempDir() + name;
}

void removeFile(const std::string &path) {
  std::remove(path.c_str());
}

void writeWavFile(const std::string &path, const std::vector<float> &frames) {
  ma_encoder encoder;
  ma_encoder_config config =
      ma_encoder_config_init(ma_encoding_format_wav, ma_format_f32, channelCount, sampleRate);
  ASSERT_EQ(ma_encoder_init_file(path.c_str(), &config, &encoder), MA_SUCCESS);

  ma_uint64 framesWritten = 0;
  EXPECT_EQ(
      ma_encoder_write_pcm_frames(
          &encoder, frames.data(), static_cast<ma_uint64>(frames.size()), &framesWritten),
      MA_SUCCESS);
  EXPECT_EQ(framesWritten, frames.size());

  ma_encoder_uninit(&encoder);
}

std::vector<uint8_t> readFileBytes(const std::string &path) {
  std::ifstream input(path, std::ios::binary);
  EXPECT_TRUE(input.is_open());
  return std::vector<uint8_t>(
      std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
}

std::string writeDurationWavFile(const std::string &name) {
  const std::string path = testFilePath(name);
  removeFile(path);
  writeWavFile(path, std::vector<float>(sampleRate / 2, 0.25F));
  return path;
}

void expectDurationNear(const audiodecoding::AudioDurationResult &result) {
  if (result.is_err()) {
    FAIL() << result.unwrap_err();
  }
  EXPECT_NEAR(result.unwrap(), expectedWavDurationSeconds, 0.001F);
}

} // namespace

TEST(AudioDecodingTest, ValidatesDurationMetadata) {
  EXPECT_TRUE(audiodecoding::isValidDuration(0.0F));
  EXPECT_FALSE(audiodecoding::isValidDuration(-1.0F));
  EXPECT_FALSE(audiodecoding::isValidDuration(std::numeric_limits<float>::infinity()));
  EXPECT_FALSE(audiodecoding::isValidDuration(std::nanf("")));
  EXPECT_TRUE(audiodecoding::isValidDuration(0.001F));
}

TEST(AudioDecodingTest, ReturnsDurationForLocalWavFile) {
  const std::string input = writeDurationWavFile("audio-duration.wav");

  expectDurationNear(audiodecoding::probeDurationWithFilePath(input));

  removeFile(input);
}

TEST(AudioDecodingTest, RejectsUnavailableDurationMetadataFromFilePath) {
  auto result = audiodecoding::probeDurationWithFilePath("");

  EXPECT_TRUE(result.is_err());
}

TEST(AudioDecodingTest, ReturnsDurationForMemoryWav) {
  const std::string input = writeDurationWavFile("audio-duration-memory.wav");
  const auto bytes = readFileBytes(input);

  expectDurationNear(audiodecoding::probeDurationWithMemory(bytes.data(), bytes.size(), 0));

  removeFile(input);
}

TEST(AudioDecodingTest, RejectsUnavailableDurationMetadataFromMemory) {
  const std::vector<uint8_t> empty;
  auto result = audiodecoding::probeDurationWithMemory(empty.data(), empty.size(), 0);

  EXPECT_TRUE(result.is_err());
}

TEST(AudioDecodingTest, RejectsInvalidDurationMetadataFromMemory) {
  const std::vector<uint8_t> invalid = {0x00, 0x01, 0x02};
  auto result = audiodecoding::probeDurationWithMemory(invalid.data(), invalid.size(), 0);

  EXPECT_TRUE(result.is_err());
}

TEST(AudioDecodingTest, ReturnsDisabledErrorForUrlProbeWhenFFmpegIsUnavailable) {
  auto result = audiodecoding::probeDurationWithUrl("https://example.com/audio.mp3", 0, {});

  EXPECT_TRUE(result.is_err());
  EXPECT_EQ(result.unwrap_err(), "FFmpeg is disabled, cannot probe duration from URL");
}

TEST(AudioDecodingTest, RejectsUnavailableDurationMetadataFromUrl) {
  auto result = audiodecoding::probeDurationWithUrl("", 0, {});

  EXPECT_TRUE(result.is_err());
  EXPECT_EQ(result.unwrap_err(), "FFmpeg is disabled, cannot probe duration from URL");
}

// NOLINTEND
