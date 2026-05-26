#include <audioapi/core/OfflineAudioContext.h>
#include <audioapi/core/sources/AudioFileSourceNode.h>
#include <audioapi/types/NodeOptions.h>
#include <audioapi/utils/AudioBuffer.hpp>
#include <gtest/gtest.h>
#include <test/src/MockAudioEventHandlerRegistry.h>

#include <cstdint>
#include <memory>
#include <vector>

using namespace audioapi;

// NOLINTBEGIN

namespace {
void appendU16LE(std::vector<uint8_t> &bytes, uint16_t value) {
  bytes.push_back(static_cast<uint8_t>(value & 0xFF));
  bytes.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
}

void appendU32LE(std::vector<uint8_t> &bytes, uint32_t value) {
  bytes.push_back(static_cast<uint8_t>(value & 0xFF));
  bytes.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
  bytes.push_back(static_cast<uint8_t>((value >> 16) & 0xFF));
  bytes.push_back(static_cast<uint8_t>((value >> 24) & 0xFF));
}

std::vector<uint8_t> makeTestWavPcm16Mono() {
  constexpr uint16_t channels = 1;
  constexpr uint32_t sampleRate = 44100;
  constexpr uint16_t bitsPerSample = 16;
  constexpr uint16_t bytesPerSample = bitsPerSample / 8;
  constexpr uint16_t blockAlign = channels * bytesPerSample;
  constexpr uint32_t byteRate = sampleRate * blockAlign;

  const std::vector<int16_t> samples = {16384, 8192, -16384, 4096};
  const uint32_t dataChunkSize = static_cast<uint32_t>(samples.size() * sizeof(int16_t));
  const uint32_t riffChunkSize = 36 + dataChunkSize;

  std::vector<uint8_t> wav;
  wav.reserve(44 + dataChunkSize);

  wav.insert(wav.end(), {'R', 'I', 'F', 'F'});
  appendU32LE(wav, riffChunkSize);
  wav.insert(wav.end(), {'W', 'A', 'V', 'E'});

  wav.insert(wav.end(), {'f', 'm', 't', ' '});
  appendU32LE(wav, 16);
  appendU16LE(wav, 1);
  appendU16LE(wav, channels);
  appendU32LE(wav, sampleRate);
  appendU32LE(wav, byteRate);
  appendU16LE(wav, blockAlign);
  appendU16LE(wav, bitsPerSample);

  wav.insert(wav.end(), {'d', 'a', 't', 'a'});
  appendU32LE(wav, dataChunkSize);

  const auto *sampleBytes = reinterpret_cast<const uint8_t *>(samples.data());
  wav.insert(wav.end(), sampleBytes, sampleBytes + dataChunkSize);

  return wav;
}
} // namespace

namespace audioapi {
class AudioFileSourceNodeTest : public ::testing::Test {
 protected:
  static constexpr int kSampleRate = 44100;
  std::shared_ptr<MockAudioEventHandlerRegistry> eventRegistry;
  std::shared_ptr<OfflineAudioContext> context;

  void SetUp() override {
    eventRegistry = std::make_shared<MockAudioEventHandlerRegistry>();
    context = std::make_shared<OfflineAudioContext>(
        2, 5 * kSampleRate, kSampleRate, eventRegistry, RuntimeRegistry{});
    context->initialize();
  }

  AudioFileSourceOptions makeFileSourceOptions() {
    AudioFileSourceOptions options;
    options.data = makeTestWavPcm16Mono();
    options.volume = 0.5f;
    return options;
  }
};

class TestableAudioFileSourceNode : public AudioFileSourceNode {
 public:
  explicit TestableAudioFileSourceNode(
      const std::shared_ptr<BaseAudioContext> &context,
      const AudioFileSourceOptions &options)
      : AudioFileSourceNode(context, options) {}

  void writeInterleavedToBufferAtOffsetForTest(
      const std::shared_ptr<DSPAudioBuffer> &processingBuffer,
      const AudioFileDecoderState &state,
      size_t destFrameOffset,
      size_t frameCount) const {
    writeInterleavedToBufferAtOffset(processingBuffer, state, destFrameOffset, frameCount);
  }
};

} // namespace audioapi

TEST_F(AudioFileSourceNodeTest, WritesDecodedAudioAtDestinationOffset) {
  auto fileSource = TestableAudioFileSourceNode(context, makeFileSourceOptions());
  auto processingBuffer = std::make_shared<DSPAudioBuffer>(6, 1, static_cast<float>(kSampleRate));

  AudioFileDecoderState state;
  state.channels = 1;
  state.interleavedBuffer = {1.0f, 0.5f};

  fileSource.writeInterleavedToBufferAtOffsetForTest(processingBuffer, state, 2, 2);

  EXPECT_FLOAT_EQ((*processingBuffer->getChannel(0))[0], 0.0f);
  EXPECT_FLOAT_EQ((*processingBuffer->getChannel(0))[1], 0.0f);
  EXPECT_FLOAT_EQ((*processingBuffer->getChannel(0))[2], 0.5f);
  EXPECT_FLOAT_EQ((*processingBuffer->getChannel(0))[3], 0.25f);
  EXPECT_FLOAT_EQ((*processingBuffer->getChannel(0))[4], 0.0f);
  EXPECT_FLOAT_EQ((*processingBuffer->getChannel(0))[5], 0.0f);
}

TEST_F(AudioFileSourceNodeTest, SecondWriteDoesNotRescaleEarlierRegion) {
  auto fileSource = TestableAudioFileSourceNode(context, makeFileSourceOptions());
  auto processingBuffer = std::make_shared<DSPAudioBuffer>(6, 1, static_cast<float>(kSampleRate));

  AudioFileDecoderState firstState;
  firstState.channels = 1;
  firstState.interleavedBuffer = {1.0f, 0.5f};

  AudioFileDecoderState secondState;
  secondState.channels = 1;
  secondState.interleavedBuffer = {0.25f, -0.25f};

  fileSource.writeInterleavedToBufferAtOffsetForTest(processingBuffer, firstState, 0, 2);
  fileSource.writeInterleavedToBufferAtOffsetForTest(processingBuffer, secondState, 2, 2);

  EXPECT_FLOAT_EQ((*processingBuffer->getChannel(0))[0], 0.5f);
  EXPECT_FLOAT_EQ((*processingBuffer->getChannel(0))[1], 0.25f);
  EXPECT_FLOAT_EQ((*processingBuffer->getChannel(0))[2], 0.125f);
  EXPECT_FLOAT_EQ((*processingBuffer->getChannel(0))[3], -0.125f);
  EXPECT_FLOAT_EQ((*processingBuffer->getChannel(0))[4], 0.0f);
  EXPECT_FLOAT_EQ((*processingBuffer->getChannel(0))[5], 0.0f);
}

// NOLINTEND
