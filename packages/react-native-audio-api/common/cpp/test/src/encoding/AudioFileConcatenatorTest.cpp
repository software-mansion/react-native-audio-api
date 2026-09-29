#include <audioapi/encoding/AudioFileConcatenator.h>
#include <audioapi/libs/miniaudio/miniaudio.h>
#include <gtest/gtest.h>
#include <test/src/utils/TestWavFile.h>

#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <limits>
#include <memory>
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

void writeOversizedWavHeader(const std::string &path) {
  std::ofstream output(path, std::ios::binary);
  ASSERT_TRUE(output.is_open());

  output.write("RIFF", 4);
  test::writeUint32LE(output, std::numeric_limits<uint32_t>::max());
  output.write("WAVE", 4);
  output.write("fmt ", 4);
  test::writeUint32LE(output, 16);
  test::writeUint16LE(output, 1);
  test::writeUint16LE(output, channelCount);
  test::writeUint32LE(output, sampleRate);
  test::writeUint32LE(output, sampleRate * channelCount);
  test::writeUint16LE(output, channelCount);
  test::writeUint16LE(output, 8);
  output.write("data", 4);
  test::writeUint32LE(output, maxOddRiffDataSize);
  output.close();

  std::error_code error;
  std::filesystem::resize_file(path, static_cast<uintmax_t>(44) + maxOddRiffDataSize, error);
  ASSERT_FALSE(error);
}

void writeWavFile(const std::string &path, const std::vector<float> &frames) {
  test::writeFloatWavFile(path, frames, sampleRate, channelCount);
}

/// Stands in for the platform encoder, which the desktop build does not have.
class FakeWavEncoder final : public AudioEncoder {
 public:
  FakeWavEncoder(const EncoderSettings &settings, bool failEncode)
      : AudioEncoder(settings), failEncode_(failEncode) {}

  /// Creates the file straight away, as the platform encoders do.
  OpenEncoderResult open(
      const StreamFormat & /*inputFormat*/,
      const EncoderOutputSpec & /*outputSpec*/,
      const std::string &filePath) override {
    filePath_ = filePath;
    std::ofstream created(filePath, std::ios::binary);
    markOpen();
    return OpenEncoderResult::Ok(filePath);
  }

  EncodeResult encode(const float *const *channels, int numFrames) override {
    if (failEncode_) {
      return EncodeResult::Err("encoder refused the frames");
    }
    frames_.insert(frames_.end(), channels[0], channels[0] + numFrames);
    return EncodeResult::Ok(static_cast<size_t>(numFrames));
  }

  CloseEncoderResult close() override {
    writeWavFile(filePath_, frames_);
    markClosed();
    return CloseEncoderResult::Ok({0.0, 0.0});
  }

  [[nodiscard]] size_t getFileSizeBytes() const override {
    return frames_.size() * sizeof(float);
  }

 private:
  bool failEncode_;
  std::vector<float> frames_;
};

std::unique_ptr<AudioEncoder> createFakeWavEncoder(const EncoderSettings &settings) {
  return std::make_unique<FakeWavEncoder>(settings, /*failEncode=*/false);
}

std::unique_ptr<AudioEncoder> createFailingWavEncoder(const EncoderSettings &settings) {
  return std::make_unique<FakeWavEncoder>(settings, /*failEncode=*/true);
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

TEST(AudioFileConcatenatorTest, ReturnsUnavailableErrorForM4AOnDesktop) {
  auto result = concatAudioFiles({"/tmp/input.m4a"}, "/tmp/output.m4a");

  EXPECT_TRUE(result.is_err());
  EXPECT_EQ(result.unwrap_err(), "concatAudioFiles remux requires iOS or Android.");
}

TEST(AudioFileConcatenatorTest, ConcatenatesWavFilesThroughTheEncoder) {
  const std::string inputA = testFilePath("audio-concat-a.wav");
  const std::string inputB = testFilePath("audio-concat-b.wav");
  const std::string output = testFilePath("audio-concat-output.wav");

  removeFile(inputA);
  removeFile(inputB);
  removeFile(output);

  writeWavFile(inputA, {0.1F, 0.2F, 0.3F});
  writeWavFile(inputB, {0.4F, 0.5F});

  auto result = concatAudioFiles({inputA, inputB}, output, createFakeWavEncoder);

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

  auto result = concatAudioFiles({input}, output, createFakeWavEncoder);

  EXPECT_TRUE(result.is_err());
  EXPECT_EQ(
      result.unwrap_err(),
      "concatAudioFiles WAV output exceeds the RIFF WAV size limit. Split the output or use a format with RF64/W64 support.");

  removeFile(input);
  removeFile(output);
}

TEST(AudioFileConcatenatorTest, RemovesTheOutputWhenEncodingFails) {
  const std::string input = testFilePath("audio-concat-failing-input.wav");
  const std::string output = testFilePath("audio-concat-failing-output.wav");

  removeFile(input);
  removeFile(output);

  writeWavFile(input, {0.1F, 0.2F});

  auto result = concatAudioFiles({input}, output, createFailingWavEncoder);

  EXPECT_TRUE(result.is_err());
  EXPECT_FALSE(std::filesystem::exists(output));

  removeFile(input);
}

TEST(AudioFileConcatenatorTest, ReturnsUnavailableErrorForWAVOnDesktop) {
  auto result = concatAudioFiles({"/tmp/input.wav"}, "/tmp/output.wav");

  EXPECT_TRUE(result.is_err());
  EXPECT_EQ(result.unwrap_err(), "concatAudioFiles WAV output requires iOS or Android.");
}

TEST(AudioFileConcatenatorTest, RejectsUnsupportedOutputFormat) {
  auto result = concatAudioFiles({"/tmp/input.ogg"}, "/tmp/output.ogg");

  EXPECT_TRUE(result.is_err());
  EXPECT_EQ(result.unwrap_err(), "concatAudioFiles supports WAV, M4A, and FLAC output.");
}

TEST(AudioFileConcatenatorTest, ReturnsUnavailableErrorForFLACOnDesktop) {
  auto result = concatAudioFiles({"/tmp/input.flac"}, "/tmp/output.flac");

  EXPECT_TRUE(result.is_err());
  EXPECT_EQ(result.unwrap_err(), "concatAudioFiles FLAC output requires iOS or Android.");
}

TEST(AudioFileConcatenatorTest, RejectsFLACOutputWithNonFlacInputs) {
  auto result = concatAudioFiles({"/tmp/input.wav"}, "/tmp/output.flac");

  EXPECT_TRUE(result.is_err());
  EXPECT_EQ(
      result.unwrap_err(),
      "concatAudioFiles FLAC output requires all input files to use the FLAC extension.");
}

TEST(AudioFileConcatenatorTest, RejectsMismatchedRemuxExtensions) {
  auto result = concatAudioFiles({"/tmp/input.m4a"}, "/tmp/output.caf");

  EXPECT_TRUE(result.is_err());
  EXPECT_EQ(result.unwrap_err(), "concatAudioFiles supports WAV, M4A, and FLAC output.");
}

// NOLINTEND
