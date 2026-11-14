#include <audioapi/decoding/AudioDecodeStream.h>
#include <audioapi/decoding/AudioDecoding.h>
#include <gtest/gtest.h>
#include <test/src/utils/TestWavFile.h>

#include <cstdio>
#include <fstream>
#include <limits>
#include <memory>
#include <string>
#include <vector>

using namespace audioapi;

// NOLINTBEGIN

namespace {

constexpr uint32_t sampleRate = 48000;
constexpr uint16_t channelCount = 2;
constexpr size_t frameCount = sampleRate / 2;

std::string testFilePath(const std::string &name) {
  return ::testing::TempDir() + name;
}

float leftSampleAt(size_t frame) {
  return static_cast<float>(frame) / static_cast<float>(frameCount);
}

/// A stereo ramp: every frame is distinguishable, and the right channel mirrors the left.
std::string writeRampWavFile(const std::string &name) {
  const std::string path = testFilePath(name);
  std::remove(path.c_str());

  std::vector<float> frames(frameCount * channelCount);
  for (size_t frame = 0; frame < frameCount; ++frame) {
    frames[frame * channelCount] = leftSampleAt(frame);
    frames[frame * channelCount + 1] = -leftSampleAt(frame);
  }
  test::writeFloatWavFile(path, frames, sampleRate, channelCount);
  return path;
}

std::vector<uint8_t> readFileBytes(const std::string &path) {
  std::ifstream input(path, std::ios::binary);
  EXPECT_TRUE(input.is_open());
  return std::vector<uint8_t>(
      std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
}

std::unique_ptr<audiodecoding::AudioDecodeStream> openStream(
    const std::string &path,
    float outputSampleRate = 0.0F) {
  auto result = audiodecoding::AudioDecodeStream::openWithFilePath(path, outputSampleRate);
  EXPECT_TRUE(result.is_ok());
  return result.is_ok() ? std::move(result).unwrap() : nullptr;
}

void appendChannels(std::vector<std::vector<float>> &channels, const AudioBuffer &buffer) {
  channels.resize(buffer.getNumberOfChannels());
  for (size_t channel = 0; channel < buffer.getNumberOfChannels(); ++channel) {
    const auto samples = buffer.getChannel(channel)->span();
    channels[channel].insert(channels[channel].end(), samples.begin(), samples.end());
  }
}

/// Reads @p stream to its end in chunks of @p chunkFrames, concatenating the channels.
std::vector<std::vector<float>> readToEnd(
    audiodecoding::AudioDecodeStream &stream,
    size_t chunkFrames) {
  std::vector<std::vector<float>> channels;
  while (true) {
    auto result = stream.read(chunkFrames);
    EXPECT_TRUE(result.is_ok());
    if (result.is_err() || result.unwrap() == nullptr) {
      return channels;
    }
    EXPECT_LE(result.unwrap()->getSize(), chunkFrames);
    appendChannels(channels, *result.unwrap());
  }
}

std::vector<std::vector<float>> decodeWhole(const std::string &path, float outputSampleRate) {
  auto result = audiodecoding::decodeWithFilePath(path, outputSampleRate);
  EXPECT_TRUE(result.is_ok());
  std::vector<std::vector<float>> channels;
  if (result.is_ok()) {
    appendChannels(channels, *result.unwrap());
  }
  return channels;
}

} // namespace

TEST(AudioDecodeStreamTest, ReportsTheFormatOfTheSource) {
  const std::string path = writeRampWavFile("decode-stream-format.wav");
  auto stream = openStream(path);
  ASSERT_NE(stream, nullptr);

  EXPECT_EQ(stream->numberOfChannels(), static_cast<int>(channelCount));
  EXPECT_FLOAT_EQ(stream->sampleRate(), static_cast<float>(sampleRate));
  EXPECT_NEAR(stream->duration(), 0.5F, 0.001F);
  EXPECT_EQ(stream->encodedSizeInBytes(), 0U);

  std::remove(path.c_str());
}

TEST(AudioDecodeStreamTest, ReportsTheRequestedSampleRate) {
  const std::string path = writeRampWavFile("decode-stream-rate.wav");
  auto stream = openStream(path, 22050.0F);
  ASSERT_NE(stream, nullptr);

  EXPECT_FLOAT_EQ(stream->sampleRate(), 22050.0F);

  std::remove(path.c_str());
}

class AudioDecodeStreamChunkTest : public ::testing::TestWithParam<size_t> {};

// The property analysis relies on: whatever the chunk size, the chunks put end to end are the
// samples a whole-file decode returns.
TEST_P(AudioDecodeStreamChunkTest, ChunksConcatenateToTheWholeDecode) {
  const std::string path = writeRampWavFile("decode-stream-chunks.wav");
  auto stream = openStream(path);
  ASSERT_NE(stream, nullptr);

  EXPECT_EQ(readToEnd(*stream, GetParam()), decodeWhole(path, 0.0F));

  std::remove(path.c_str());
}

TEST_P(AudioDecodeStreamChunkTest, ChunksConcatenateToTheWholeDecodeWhenResampling) {
  const std::string path = writeRampWavFile("decode-stream-chunks-resampled.wav");
  constexpr float outputSampleRate = 22050.0F;
  auto stream = openStream(path, outputSampleRate);
  ASSERT_NE(stream, nullptr);

  EXPECT_EQ(readToEnd(*stream, GetParam()), decodeWhole(path, outputSampleRate));

  std::remove(path.c_str());
}

INSTANTIATE_TEST_SUITE_P(
    ChunkSizes,
    AudioDecodeStreamChunkTest,
    ::testing::Values(
        1,
        1000,
        decoding::AudioDecoderBackend::CHUNK_SIZE,
        // not a multiple of the decoder's own chunk
        decoding::AudioDecoderBackend::CHUNK_SIZE + 904,
        // longer than the source: one short chunk
        frameCount + 10));

TEST(AudioDecodeStreamTest, ReturnsAShortLastChunkThenNoBuffer) {
  const std::string path = writeRampWavFile("decode-stream-end.wav");
  auto stream = openStream(path);
  ASSERT_NE(stream, nullptr);
  constexpr size_t chunkFrames = 10000;

  auto first = stream->read(chunkFrames);
  ASSERT_TRUE(first.is_ok());
  ASSERT_NE(first.unwrap(), nullptr);
  EXPECT_EQ(first.unwrap()->getSize(), chunkFrames);

  auto second = stream->read(chunkFrames);
  ASSERT_TRUE(second.is_ok());
  ASSERT_NE(second.unwrap(), nullptr);
  EXPECT_EQ(second.unwrap()->getSize(), chunkFrames);

  auto last = stream->read(chunkFrames);
  ASSERT_TRUE(last.is_ok());
  ASSERT_NE(last.unwrap(), nullptr);
  EXPECT_EQ(last.unwrap()->getSize(), frameCount - 2 * chunkFrames);

  auto afterEnd = stream->read(chunkFrames);
  ASSERT_TRUE(afterEnd.is_ok());
  EXPECT_EQ(afterEnd.unwrap(), nullptr);

  std::remove(path.c_str());
}

// A request far longer than the source must return the source, whichever branch sizes the
// buffer: a plain large request, and one past what a duration estimate is trusted for.
TEST(AudioDecodeStreamTest, ReturnsTheWholeSourceForAnOversizedRequest) {
  const std::string path = writeRampWavFile("decode-stream-oversized.wav");
  constexpr size_t tenMinutes = static_cast<size_t>(sampleRate) * 60 * 10;

  for (const size_t request : {tenMinutes, std::numeric_limits<size_t>::max()}) {
    auto stream = openStream(path);
    ASSERT_NE(stream, nullptr);

    auto result = stream->read(request);
    ASSERT_TRUE(result.is_ok());
    ASSERT_NE(result.unwrap(), nullptr);
    EXPECT_EQ(result.unwrap()->getSize(), frameCount);

    auto afterEnd = stream->read(request);
    ASSERT_TRUE(afterEnd.is_ok());
    EXPECT_EQ(afterEnd.unwrap(), nullptr);
  }

  std::remove(path.c_str());
}

TEST(AudioDecodeStreamTest, SeekMovesTheReadPosition) {
  const std::string path = writeRampWavFile("decode-stream-seek.wav");
  auto stream = openStream(path);
  ASSERT_NE(stream, nullptr);
  constexpr size_t targetFrame = frameCount / 2;

  ASSERT_TRUE(stream->read(1000).is_ok());
  auto seek = stream->seek(static_cast<double>(targetFrame) / sampleRate);
  ASSERT_TRUE(seek.is_ok()) << seek.unwrap_err();

  auto result = stream->read(16);
  ASSERT_TRUE(result.is_ok());
  ASSERT_NE(result.unwrap(), nullptr);
  EXPECT_FLOAT_EQ((*result.unwrap()->getChannel(0))[0], leftSampleAt(targetFrame));
  EXPECT_FLOAT_EQ((*result.unwrap()->getChannel(1))[0], -leftSampleAt(targetFrame));

  std::remove(path.c_str());
}

TEST(AudioDecodeStreamTest, SeekBackAfterTheEndReadsAgain) {
  const std::string path = writeRampWavFile("decode-stream-rewind.wav");
  auto stream = openStream(path);
  ASSERT_NE(stream, nullptr);

  const auto firstPass = readToEnd(*stream, 4096);
  ASSERT_TRUE(stream->seek(0.0).is_ok());

  EXPECT_EQ(readToEnd(*stream, 4096), firstPass);

  std::remove(path.c_str());
}

TEST(AudioDecodeStreamTest, RejectsAnInvalidSeekPosition) {
  const std::string path = writeRampWavFile("decode-stream-bad-seek.wav");
  auto stream = openStream(path);
  ASSERT_NE(stream, nullptr);

  EXPECT_TRUE(stream->seek(-1.0).is_err());
  EXPECT_TRUE(stream->seek(std::numeric_limits<double>::quiet_NaN()).is_err());

  std::remove(path.c_str());
}

TEST(AudioDecodeStreamTest, RejectsAReadOfNoFrame) {
  const std::string path = writeRampWavFile("decode-stream-zero.wav");
  auto stream = openStream(path);
  ASSERT_NE(stream, nullptr);

  EXPECT_TRUE(stream->read(0).is_err());

  std::remove(path.c_str());
}

TEST(AudioDecodeStreamTest, RejectsReadAndSeekOnceClosed) {
  const std::string path = writeRampWavFile("decode-stream-closed.wav");
  auto stream = openStream(path);
  ASSERT_NE(stream, nullptr);

  stream->close();
  stream->close();

  EXPECT_TRUE(stream->read(1000).is_err());
  EXPECT_TRUE(stream->seek(0.0).is_err());

  std::remove(path.c_str());
}

TEST(AudioDecodeStreamTest, ReadsFromAMemoryBlock) {
  const std::string path = writeRampWavFile("decode-stream-memory.wav");
  const auto bytes = readFileBytes(path);

  auto result = audiodecoding::AudioDecodeStream::openWithMemoryBlock(bytes, 0);
  ASSERT_TRUE(result.is_ok()) << result.unwrap_err();
  auto stream = std::move(result).unwrap();

  EXPECT_EQ(stream->encodedSizeInBytes(), bytes.size());
  EXPECT_EQ(readToEnd(*stream, 5000), decodeWhole(path, 0.0F));

  std::remove(path.c_str());
}

TEST(AudioDecodeStreamTest, RejectsASourceThatCannotBeDecoded) {
  const std::vector<uint8_t> invalid = {0x00, 0x01, 0x02};

  EXPECT_TRUE(audiodecoding::AudioDecodeStream::openWithFilePath("", 0).is_err());
  EXPECT_TRUE(audiodecoding::AudioDecodeStream::openWithMemoryBlock({}, 0).is_err());
  EXPECT_TRUE(audiodecoding::AudioDecodeStream::openWithMemoryBlock(invalid, 0).is_err());
}

// NOLINTEND
