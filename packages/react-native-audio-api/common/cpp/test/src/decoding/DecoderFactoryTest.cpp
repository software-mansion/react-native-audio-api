#include <audioapi/decoding/DecoderFactory.h>
#include <audioapi/decoding/DecoderSource.h>
#include <audioapi/libs/miniaudio/miniaudio.h>
#include <gtest/gtest.h>

#include <cstdio>
#include <fstream>
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

std::vector<uint8_t> readFileBytes(const std::string &path) {
  std::ifstream input(path, std::ios::binary);
  EXPECT_TRUE(input.is_open());
  return std::vector<uint8_t>(
      std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
}

} // namespace

TEST(DecoderFactoryTest, OpensLocalWavFile) {
  const std::string path = testFilePath("decoder-factory.wav");
  removeFile(path);
  writeWavFile(path, std::vector<float>(sampleRate / 2, 0.25F));

  auto result = decoding::createDecoder(decoding::LocalFileSource{.path = path, .sampleRate = 0});
  ASSERT_TRUE(result.is_ok()) << result.unwrap_err();

  auto decoder = std::move(result).unwrap();
  EXPECT_TRUE(decoder->isOpen());
  EXPECT_EQ(decoder->outputChannels(), static_cast<int>(channelCount));
  EXPECT_EQ(decoder->outputSampleRate(), static_cast<int>(sampleRate));
  EXPECT_GT(decoder->getTotalPcmFrameCount(), 0U);
  decoder->close();

  removeFile(path);
}

TEST(DecoderFactoryTest, OpensEncodedMemoryWav) {
  const std::string path = testFilePath("decoder-factory-memory.wav");
  removeFile(path);
  writeWavFile(path, std::vector<float>(sampleRate / 4, 0.5F));
  const auto bytes = readFileBytes(path);

  auto result =
      decoding::createDecoder(decoding::EncodedMemorySource{.data = bytes, .sampleRate = 0});
  ASSERT_TRUE(result.is_ok()) << result.unwrap_err();

  auto decoder = std::move(result).unwrap();
  EXPECT_TRUE(decoder->isOpen());
  EXPECT_GT(decoder->getDurationInSeconds(), 0.0f);
  decoder->close();

  removeFile(path);
}

TEST(DecoderFactoryTest, RejectsEmptyLocalFilePath) {
  auto result = decoding::createDecoder(decoding::LocalFileSource{});
  EXPECT_TRUE(result.is_err());
}

TEST(DecoderFactoryTest, RejectsEmptyEncodedMemory) {
  auto result = decoding::createDecoder(decoding::EncodedMemorySource{});
  EXPECT_TRUE(result.is_err());
}

TEST(DecoderFactoryTest, RejectsRemoteUrlWhenFfmpegDisabled) {
  auto result = decoding::createDecoder(
      decoding::RemoteUrlSource{.url = "https://example.com/audio.mp3", .sampleRate = 0});
  EXPECT_TRUE(result.is_err());
  EXPECT_EQ(result.unwrap_err(), "FFmpeg is disabled, cannot decode remote URL");
}

TEST(DecoderFactoryTest, RejectsLocalM3u8WhenFfmpegDisabled) {
  auto result = decoding::createDecoder(
      decoding::LocalFileSource{.path = "/tmp/playlist.m3u8", .sampleRate = 0});
  EXPECT_TRUE(result.is_err());
  EXPECT_EQ(result.unwrap_err(), "FFmpeg is disabled, cannot decode local HLS (.m3u8) playlists");
}

TEST(DecoderFactoryTest, OpensRawPcmSource) {
  std::vector<uint8_t> pcmBytes(2);
  pcmBytes[0] = 0x00;
  pcmBytes[1] = 0x00;

  auto result = decoding::createDecoder(
      decoding::RawPcmSource{
          .data = std::move(pcmBytes),
          .sampleRate = 44100,
          .channelCount = 1,
          .interleaved = true});
  ASSERT_TRUE(result.is_ok()) << result.unwrap_err();

  auto decoder = std::move(result).unwrap();
  EXPECT_EQ(decoder->outputChannels(), 1);
  EXPECT_EQ(decoder->outputSampleRate(), 44100);
  EXPECT_EQ(decoder->getTotalPcmFrameCount(), 1U);

  float sample = 0.0f;
  EXPECT_EQ(decoder->readPcmFrames(&sample, 1), 1U);
  decoder->close();
}

// NOLINTEND
