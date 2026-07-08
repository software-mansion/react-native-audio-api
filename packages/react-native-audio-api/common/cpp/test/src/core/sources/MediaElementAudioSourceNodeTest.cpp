#include <audioapi/core/OfflineAudioContext.h>
#include <audioapi/core/destinations/AudioDestinationNode.h>
#include <audioapi/core/sources/AudioFileSourceNode.h>
#include <audioapi/core/sources/MediaElementAudioSourceNode.h>
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

  const std::vector<int16_t> samples = {0, 16384, -16384, 8192};
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

class MediaElementAudioSourceNodeTest : public ::testing::Test {
 protected:
  static constexpr int kSampleRate = 44100;
  std::shared_ptr<MockAudioEventHandlerRegistry> eventRegistry;
  std::shared_ptr<OfflineAudioContext> context;
  std::shared_ptr<AudioDestinationNode> destination;

  void SetUp() override {
    eventRegistry = std::make_shared<MockAudioEventHandlerRegistry>();
    context = std::make_shared<OfflineAudioContext>(
        2, 5 * kSampleRate, kSampleRate, eventRegistry, RuntimeRegistry{});
    destination = std::make_shared<AudioDestinationNode>(context);
    context->initialize(destination.get());
  }

  std::shared_ptr<AudioFileSourceNode> createFileSource() {
    AudioFileSourceOptions options;
    options.data = makeTestWavPcm16Mono();
    auto fileSource = std::make_shared<AudioFileSourceNode>(context, options);
    EXPECT_NE(fileSource, nullptr);
    return fileSource;
  }
};

class TestableMediaElementAudioSourceNode : public MediaElementAudioSourceNode {
 public:
  explicit TestableMediaElementAudioSourceNode(
      const std::shared_ptr<BaseAudioContext> &context,
      AudioFileSourceNode *fileSource,
      const MediaElementAudioSourceOptions &options)
      : MediaElementAudioSourceNode(context, fileSource, options) {}

  const DSPAudioBuffer *processForTest(int framesToProcess) {
    processNode(framesToProcess);
    return getOutputBuffer().get();
  }
};

TEST_F(MediaElementAudioSourceNodeTest, StaleBindingReleaseIsIgnored) {
  auto fileSource = createFileSource();
  ASSERT_NE(fileSource, nullptr);

  fileSource->bindMediaElementSource(1);
  fileSource->bindMediaElementSource(2);

  fileSource->releaseMediaElementSource(1);

  EXPECT_TRUE(fileSource->isRoutedThroughMediaElement());
  EXPECT_TRUE(fileSource->isCurrentMediaElementSource(2));

  fileSource->releaseMediaElementSource(2);

  EXPECT_FALSE(fileSource->isRoutedThroughMediaElement());
  EXPECT_FALSE(fileSource->isCurrentMediaElementSource(2));
}

TEST_F(MediaElementAudioSourceNodeTest, DisconnectingLastOutputReleasesMediaBinding) {
  auto fileSource = createFileSource();
  ASSERT_NE(fileSource, nullptr);

  auto media = std::make_shared<MediaElementAudioSourceNode>(
      context,
      fileSource.get(),
      MediaElementAudioSourceOptions(static_cast<int>(fileSource->getChannelCount())));

  ASSERT_TRUE(fileSource->isRoutedThroughMediaElement());

  media->onOutputsDisconnected();

  EXPECT_FALSE(fileSource->isRoutedThroughMediaElement());
}

TEST_F(MediaElementAudioSourceNodeTest, StaleMediaNodeOutputsSilence) {
  auto fileSource = createFileSource();
  ASSERT_NE(fileSource, nullptr);

  auto mediaA = std::make_shared<TestableMediaElementAudioSourceNode>(
      context,
      fileSource.get(),
      MediaElementAudioSourceOptions(static_cast<int>(fileSource->getChannelCount())));
  auto mediaB = std::make_shared<TestableMediaElementAudioSourceNode>(
      context,
      fileSource.get(),
      MediaElementAudioSourceOptions(static_cast<int>(fileSource->getChannelCount())));

  ASSERT_TRUE(fileSource->isCurrentMediaElementSource(mediaB->getBindingId()));

  constexpr int kFrames = 8;
  const DSPAudioBuffer *out = mediaA->processForTest(kFrames);
  ASSERT_NE(out, nullptr);

  for (int i = 0; i < kFrames; ++i) {
    EXPECT_FLOAT_EQ((*out->getChannel(0))[i], 0.0f);
  }
}

// NOLINTEND
