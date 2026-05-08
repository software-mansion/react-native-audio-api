#include <audioapi/core/utils/AudioFileConcatenator.h>
#include <audioapi/libs/miniaudio/miniaudio.h>
#include <gtest/gtest.h>

#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <limits>
#include <string>
#include <vector>

using namespace audioapi;

// NOLINTBEGIN

namespace {

constexpr ma_uint32 sampleRate = 48000;
constexpr ma_uint32 channelCount = 1;
constexpr uint32_t maxOddRiffDataSize = std::numeric_limits<uint32_t>::max() - 36;

std::string testFilePath(const std::string &name) {
  return ::testing::TempDir() + name;
}

void removeFile(const std::string &path) {
  std::remove(path.c_str());
}

void writeUint16LE(std::ofstream &output, uint16_t value) {
  output.put(static_cast<char>(value & 0xFF));
  output.put(static_cast<char>((value >> 8) & 0xFF));
}

void writeUint32LE(std::ofstream &output, uint32_t value) {
  output.put(static_cast<char>(value & 0xFF));
  output.put(static_cast<char>((value >> 8) & 0xFF));
  output.put(static_cast<char>((value >> 16) & 0xFF));
  output.put(static_cast<char>((value >> 24) & 0xFF));
}

void writeOversizedWavHeader(const std::string &path) {
  std::ofstream output(path, std::ios::binary);
  ASSERT_TRUE(output.is_open());

  output.write("RIFF", 4);
  writeUint32LE(output, std::numeric_limits<uint32_t>::max());
  output.write("WAVE", 4);
  output.write("fmt ", 4);
  writeUint32LE(output, 16);
  writeUint16LE(output, 1);
  writeUint16LE(output, channelCount);
  writeUint32LE(output, sampleRate);
  writeUint32LE(output, sampleRate * channelCount);
  writeUint16LE(output, channelCount);
  writeUint16LE(output, 8);
  output.write("data", 4);
  writeUint32LE(output, maxOddRiffDataSize);
  output.close();

  std::error_code error;
  std::filesystem::resize_file(path, static_cast<uintmax_t>(44) + maxOddRiffDataSize, error);
  ASSERT_FALSE(error);
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

std::vector<float> readWavFile(const std::string &path) {
  ma_decoder decoder;
  ma_decoder_config config = ma_decoder_config_init(ma_format_f32, channelCount, sampleRate);
  EXPECT_EQ(ma_decoder_init_file(path.c_str(), &config, &decoder), MA_SUCCESS);

  std::vector<float> frames;
  std::vector<float> chunk(32);
  while (true) {
    ma_uint64 framesRead = 0;
    ma_result result =
        ma_decoder_read_pcm_frames(&decoder, chunk.data(), chunk.size(), &framesRead);
    EXPECT_TRUE(result == MA_SUCCESS || result == MA_AT_END);

    if (framesRead == 0) {
      break;
    }

    frames.insert(frames.end(), chunk.data(), chunk.data() + framesRead);

    if (result == MA_AT_END) {
      break;
    }
  }

  ma_decoder_uninit(&decoder);
  return frames;
}

} // namespace

TEST(AudioFileConcatenatorTest, NormalizesFileUrls) {
  EXPECT_EQ(normalizeFilePath("file:///tmp/audio%20segment.m4a"), "/tmp/audio segment.m4a");
}

TEST(AudioFileConcatenatorTest, KeepsFilesystemPaths) {
  EXPECT_EQ(normalizeFilePath("/tmp/audio%20segment.m4a"), "/tmp/audio segment.m4a");
}

TEST(AudioFileConcatenatorTest, RejectsEmptyInputList) {
  auto result = concatAudioFiles({}, "/tmp/output.m4a");

  EXPECT_TRUE(result.is_err());
  EXPECT_EQ(result.unwrap_err(), "concatAudioFiles requires at least one input path.");
}

TEST(AudioFileConcatenatorTest, RejectsEmptyOutputPath) {
  auto result = concatAudioFiles({"/tmp/input.m4a"}, "");

  EXPECT_TRUE(result.is_err());
  EXPECT_EQ(result.unwrap_err(), "concatAudioFiles requires an output path.");
}

TEST(AudioFileConcatenatorTest, RejectsEmptyFileUrlOutputPath) {
  auto result = concatAudioFiles({"/tmp/input.m4a"}, "file://");

  EXPECT_TRUE(result.is_err());
  EXPECT_EQ(result.unwrap_err(), "concatAudioFiles requires an output path.");
}

TEST(AudioFileConcatenatorTest, RejectsEmptyFileUrlInputPath) {
  auto result = concatAudioFiles({"file://"}, "/tmp/output.m4a");

  EXPECT_TRUE(result.is_err());
  EXPECT_EQ(result.unwrap_err(), "concatAudioFiles input path at index 0 is empty.");
}

TEST(AudioFileConcatenatorTest, RejectsNonFileInputProtocol) {
  auto result = concatAudioFiles({"http://example.com/input.m4a"}, "/tmp/output.m4a");

  EXPECT_TRUE(result.is_err());
  EXPECT_EQ(
      result.unwrap_err(),
      "concatAudioFiles input path at index 0 must be a local file path or file:// URL.");
}

TEST(AudioFileConcatenatorTest, RejectsNonFileOutputProtocol) {
  auto result = concatAudioFiles({"/tmp/input.m4a"}, "pipe:output.m4a");

  EXPECT_TRUE(result.is_err());
  EXPECT_EQ(
      result.unwrap_err(),
      "concatAudioFiles output path must be a local file path or file:// URL.");
}

TEST(AudioFileConcatenatorTest, ReturnsDisabledErrorForM4AWhenFFmpegIsUnavailable) {
  auto result = concatAudioFiles({"/tmp/input.m4a"}, "/tmp/output.m4a");

  EXPECT_TRUE(result.is_err());
  EXPECT_EQ(result.unwrap_err(), "FFmpeg is disabled, cannot concatenate M4A/MP4 audio files.");
}

TEST(AudioFileConcatenatorTest, RejectsFLACOutputAsUnsupported) {
  auto result = concatAudioFiles({"/tmp/input.flac"}, "/tmp/output.flac");

  EXPECT_TRUE(result.is_err());
  EXPECT_EQ(
      result.unwrap_err(),
      "concatAudioFiles supports WAV output with miniaudio and M4A/MP4 output with FFmpeg.");
}

TEST(AudioFileConcatenatorTest, ConcatenatesWavFilesWithMiniaudioWhenFFmpegIsUnavailable) {
  const std::string inputA = testFilePath("audio-concat-a.wav");
  const std::string inputB = testFilePath("audio-concat-b.wav");
  const std::string output = testFilePath("audio-concat-output.wav");

  removeFile(inputA);
  removeFile(inputB);
  removeFile(output);

  writeWavFile(inputA, {0.1F, 0.2F, 0.3F});
  writeWavFile(inputB, {0.4F, 0.5F});

  auto result = concatAudioFiles({inputA, inputB}, output);

  if (result.is_err()) {
    FAIL() << result.unwrap_err();
  }
  EXPECT_EQ(result.unwrap(), output);
  EXPECT_EQ(readWavFile(output), (std::vector<float>{0.1F, 0.2F, 0.3F, 0.4F, 0.5F}));

  removeFile(inputA);
  removeFile(inputB);
  removeFile(output);
}

TEST(AudioFileConcatenatorTest, RejectsWavOutputThatWouldExceedRiffLimit) {
  const std::string input = testFilePath("audio-concat-oversized.wav");
  const std::string output = testFilePath("audio-concat-oversized-output.wav");

  removeFile(input);
  removeFile(output);

  writeOversizedWavHeader(input);

  auto result = concatAudioFiles({input}, output);

  EXPECT_TRUE(result.is_err());
  EXPECT_EQ(
      result.unwrap_err(),
      "concatAudioFiles WAV output exceeds the RIFF WAV size limit. Split the output or use a format with RF64/W64 support.");

  removeFile(input);
  removeFile(output);
}

TEST(AudioFileConcatenatorTest, RejectsUnsupportedOutputFormatWhenFFmpegIsUnavailable) {
  auto result = concatAudioFiles({"/tmp/input.ogg"}, "/tmp/output.ogg");

  EXPECT_TRUE(result.is_err());
  EXPECT_EQ(
      result.unwrap_err(),
      "concatAudioFiles supports WAV output with miniaudio and M4A/MP4 output with FFmpeg.");
}

// NOLINTEND
