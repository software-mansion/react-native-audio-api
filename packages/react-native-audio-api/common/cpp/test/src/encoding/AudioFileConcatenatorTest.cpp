#include <audioapi/encoding/AdtsHeader.h>
#include <audioapi/encoding/AudioFileConcatenator.h>
#include <audioapi/libs/miniaudio/miniaudio.h>
#include <gtest/gtest.h>
#include <test/src/utils/TestWavFile.h>

#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
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

enum class EncodeOutcome { ACCEPTS_FRAMES, REFUSES_FRAMES };

/// Stands in for the platform encoder, which the desktop build does not have.
class FakeWavEncoder final : public AudioEncoder {
 public:
  FakeWavEncoder(const EncoderSettings &settings, EncodeOutcome outcome)
      : AudioEncoder(settings), outcome_(outcome) {}

  /// Creates the file on open like the platform encoders, so a concat that fails after this
  /// point has a partial output to clean up.
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
    if (outcome_ == EncodeOutcome::REFUSES_FRAMES) {
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
  EncodeOutcome outcome_;
  std::vector<float> frames_;
};

CreateEncoderResult createFakeWavEncoder(const EncoderSettings &settings) {
  return CreateEncoderResult::Ok(
      std::make_unique<FakeWavEncoder>(settings, EncodeOutcome::ACCEPTS_FRAMES));
}

CreateEncoderResult createFailingWavEncoder(const EncoderSettings &settings) {
  return CreateEncoderResult::Ok(
      std::make_unique<FakeWavEncoder>(settings, EncodeOutcome::REFUSES_FRAMES));
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

/// One ADTS frame per payload byte count, so a stream's framing is known to the byte.
std::vector<char> makeAdtsStream(
    uint8_t samplingFrequencyIndex,
    int channelCount,
    const std::vector<size_t> &payloadSizes) {
  std::vector<char> bytes;
  for (size_t payload : payloadSizes) {
    const auto header = *adts::makeHeader(samplingFrequencyIndex, channelCount, payload);
    bytes.insert(bytes.end(), header.begin(), header.end());
    bytes.insert(bytes.end(), payload, static_cast<char>(0xAB));
  }
  return bytes;
}

void writeBytes(const std::string &path, const std::vector<char> &bytes) {
  std::ofstream output(path, std::ios::binary);
  ASSERT_TRUE(output.is_open());
  output.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
}

std::vector<char> readBytes(const std::string &path) {
  std::ifstream input(path, std::ios::binary);
  return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
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
  EXPECT_EQ(result.unwrap_err(), "concatAudioFiles supports WAV, M4A, FLAC, and ADTS output.");
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
  EXPECT_EQ(result.unwrap_err(), "concatAudioFiles supports WAV, M4A, FLAC, and ADTS output.");
}

TEST(AudioFileConcatenatorTest, AppendsAdtsFramesByteForByte) {
  const std::string inputA = testFilePath("audio-concat-a.aac");
  const std::string inputB = testFilePath("audio-concat-b.aac");
  const std::string output = testFilePath("audio-concat-output.aac");
  removeFile(inputA);
  removeFile(inputB);
  removeFile(output);

  const auto streamA = makeAdtsStream(4, 2, {100, 250});
  const auto streamB = makeAdtsStream(4, 2, {80});
  writeBytes(inputA, streamA);
  writeBytes(inputB, streamB);

  auto result = concatAudioFiles({inputA, inputB}, output);

  if (result.is_err()) {
    FAIL() << result.unwrap_err();
  }
  EXPECT_EQ(result.unwrap(), output);
  std::vector<char> expected = streamA;
  expected.insert(expected.end(), streamB.begin(), streamB.end());
  EXPECT_EQ(readBytes(output), expected);

  removeFile(inputA);
  removeFile(inputB);
  removeFile(output);
}

TEST(AudioFileConcatenatorTest, DropsATrailingPartialAdtsFrame) {
  const std::string inputA = testFilePath("audio-concat-cut.aac");
  const std::string inputB = testFilePath("audio-concat-next.aac");
  const std::string output = testFilePath("audio-concat-cut-output.aac");
  removeFile(inputA);
  removeFile(inputB);
  removeFile(output);

  // A recording killed mid-frame.
  constexpr size_t lastPayloadBytes = 50;
  constexpr size_t bytesLostToTheKill = 10;
  auto streamA = makeAdtsStream(4, 1, {100, lastPayloadBytes});
  const auto completeBytes = streamA.size() - lastPayloadBytes - adts::HEADER_SIZE;
  streamA.resize(streamA.size() - bytesLostToTheKill);
  const auto streamB = makeAdtsStream(4, 1, {60});
  writeBytes(inputA, streamA);
  writeBytes(inputB, streamB);

  auto result = concatAudioFiles({inputA, inputB}, output);

  if (result.is_err()) {
    FAIL() << result.unwrap_err();
  }
  std::vector<char> expected(streamA.begin(), streamA.begin() + completeBytes);
  expected.insert(expected.end(), streamB.begin(), streamB.end());
  EXPECT_EQ(readBytes(output), expected);

  removeFile(inputA);
  removeFile(inputB);
  removeFile(output);
}

TEST(AudioFileConcatenatorTest, RejectsAdtsInputsWithDifferentLayouts) {
  const std::string inputA = testFilePath("audio-concat-stereo.aac");
  const std::string inputB = testFilePath("audio-concat-mono.aac");
  const std::string output = testFilePath("audio-concat-layout-output.aac");
  removeFile(inputA);
  removeFile(inputB);
  removeFile(output);

  writeBytes(inputA, makeAdtsStream(4, 2, {100}));
  writeBytes(inputB, makeAdtsStream(4, 1, {100}));

  auto result = concatAudioFiles({inputA, inputB}, output);

  EXPECT_TRUE(result.is_err());
  EXPECT_EQ(result.unwrap_err(), "Input file '" + inputB + "' uses a different channel count.");
  EXPECT_FALSE(std::filesystem::exists(output));

  removeFile(inputA);
  removeFile(inputB);
}

TEST(AudioFileConcatenatorTest, RejectsAdtsOutputWithNonAdtsInputs) {
  const std::string input = testFilePath("audio-concat-not-adts.aac");
  removeFile(input);
  writeBytes(input, {'R', 'I', 'F', 'F', 0, 0, 0, 0, 'W', 'A', 'V', 'E'});

  auto wrongExtension = concatAudioFiles({"/tmp/input.m4a"}, "/tmp/output.aac");
  EXPECT_TRUE(wrongExtension.is_err());
  EXPECT_EQ(
      wrongExtension.unwrap_err(),
      "concatAudioFiles ADTS output requires all input files to use the AAC extension.");

  auto notAStream = concatAudioFiles({input}, testFilePath("audio-concat-not-adts-output.aac"));
  EXPECT_TRUE(notAStream.is_err());
  EXPECT_EQ(
      notAStream.unwrap_err(),
      "Input file '" + input + "' is not an ADTS stream (no frame at byte 0).");

  removeFile(input);
}

namespace {

std::vector<std::string> tempFilesNamedLike(const std::string &prefix) {
  std::vector<std::string> names;
  for (const auto &entry : std::filesystem::directory_iterator(::testing::TempDir())) {
    const std::string name = entry.path().filename().string();
    if (name.rfind(prefix, 0) == 0) {
      names.push_back(name);
    }
  }
  return names;
}

} // namespace

TEST(AudioFileConcatenatorTest, ConcatenatesInPlaceWhenTheOutputIsAnInput) {
  const std::string recording = testFilePath("audio-concat-inplace.aac");
  const std::string segment = testFilePath("audio-concat-inplace-segment.aac");
  removeFile(recording);
  removeFile(segment);

  const auto streamA = makeAdtsStream(4, 2, {100, 250});
  const auto streamB = makeAdtsStream(4, 2, {80});
  writeBytes(recording, streamA);
  writeBytes(segment, streamB);

  auto result = concatAudioFiles({recording, segment}, recording);

  if (result.is_err()) {
    FAIL() << result.unwrap_err();
  }
  std::vector<char> expected = streamA;
  expected.insert(expected.end(), streamB.begin(), streamB.end());
  EXPECT_EQ(readBytes(recording), expected);
  EXPECT_EQ(tempFilesNamedLike("audio-concat-inplace").size(), 2U);

  removeFile(recording);
  removeFile(segment);
}

TEST(AudioFileConcatenatorTest, KeepsAnExistingOutputWhenConcatenationFails) {
  const std::string input = testFilePath("audio-concat-keep-input.wav");
  const std::string output = testFilePath("audio-concat-keep-output.wav");
  removeFile(input);
  removeFile(output);

  writeWavFile(input, {0.1F, 0.2F});
  const std::vector<char> previous = {'o', 'l', 'd'};
  writeBytes(output, previous);

  auto result = concatAudioFiles({input}, output, createFailingWavEncoder);

  EXPECT_TRUE(result.is_err());
  EXPECT_EQ(readBytes(output), previous);
  EXPECT_EQ(tempFilesNamedLike("audio-concat-keep-output").size(), 1U);

  removeFile(input);
  removeFile(output);
}

TEST(AudioFileConcatenatorTest, LeavesNoStagingFileAfterAFailedAdtsConcat) {
  const std::string inputA = testFilePath("audio-concat-nostage-a.aac");
  const std::string inputB = testFilePath("audio-concat-nostage-b.aac");
  const std::string output = testFilePath("audio-concat-nostage-output.aac");
  removeFile(inputA);
  removeFile(inputB);
  removeFile(output);

  writeBytes(inputA, makeAdtsStream(4, 2, {100}));
  writeBytes(inputB, makeAdtsStream(4, 1, {100}));

  auto result = concatAudioFiles({inputA, inputB}, output);

  EXPECT_TRUE(result.is_err());
  EXPECT_TRUE(tempFilesNamedLike("audio-concat-nostage-output").empty());

  removeFile(inputA);
  removeFile(inputB);
}

// NOLINTEND
