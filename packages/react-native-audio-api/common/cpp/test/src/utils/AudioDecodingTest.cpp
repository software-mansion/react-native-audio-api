#include <audioapi/core/utils/AudioDecoding.hpp>
#include <audioapi/libs/miniaudio/miniaudio.h>
#include <gtest/gtest.h>

#include <cmath>
#include <cstdio>
#include <limits>
#include <string>
#include <vector>

using namespace audioapi;

// NOLINTBEGIN

namespace {

constexpr ma_uint32 sampleRate = 48000;
constexpr ma_uint32 channelCount = 1;

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

} // namespace

TEST(AudioDecodingTest, ValidatesDurationMetadata) {
  EXPECT_FALSE(audiodecoding::isValidDuration(0.0F));
  EXPECT_FALSE(audiodecoding::isValidDuration(-1.0F));
  EXPECT_FALSE(audiodecoding::isValidDuration(std::numeric_limits<float>::infinity()));
  EXPECT_FALSE(audiodecoding::isValidDuration(std::nanf("")));
  EXPECT_TRUE(audiodecoding::isValidDuration(0.001F));
}

TEST(AudioDecodingTest, ReturnsDurationForLocalWavFile) {
  const std::string input = testFilePath("audio-duration.wav");
  removeFile(input);

  writeWavFile(input, std::vector<float>(sampleRate / 2, 0.25F));

  auto result = audiodecoding::getDurationWithFilePath(input);

  if (result.is_err()) {
    FAIL() << result.unwrap_err();
  }
  EXPECT_NEAR(result.unwrap(), 0.5F, 0.001F);

  removeFile(input);
}

TEST(AudioDecodingTest, RejectsUnavailableDurationMetadata) {
  auto result = audiodecoding::getDurationWithFilePath("");

  EXPECT_TRUE(result.is_err());
}

// NOLINTEND
