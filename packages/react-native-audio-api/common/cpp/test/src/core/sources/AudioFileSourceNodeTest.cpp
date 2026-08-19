#include <audioapi/core/OfflineAudioContext.h>
#include <audioapi/core/destinations/AudioDestinationNode.h>
#include <audioapi/core/sources/AudioFileSourceNode.h>
#include <audioapi/events/AudioEventPayload.h>
#include <audioapi/types/NodeOptions.h>
#include <gmock/gmock.h>
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

// Same fixture as MediaElementAudioSourceNodeTest.cpp — a tiny in-memory
// PCM16 mono WAV, just enough for AudioFileSourceNode's decoder to open
// successfully. The tests below exercise updateBufferingState() directly
// rather than through real playback, since forcing genuine decoder-daemon
// starvation deterministically would mean racing a real background thread.
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

class TestableAudioFileSourceNode : public AudioFileSourceNode {
 public:
  using AudioFileSourceNode::AudioFileSourceNode;

  void callUpdateBufferingState(bool hasData, int framesToProcess) {
    updateBufferingState(hasData, framesToProcess);
  }
};

class AudioFileSourceNodeTest : public ::testing::Test {
 protected:
  static constexpr int kSampleRate = 44100;
  // Mirrors ON_BUFFERING_STATE_DEBOUNCE_INTERVAL in AudioFileSourceNode.h.
  static constexpr int kThresholdFrames =
      static_cast<int>(kSampleRate * ON_BUFFERING_STATE_DEBOUNCE_INTERVAL);

  std::shared_ptr<MockAudioEventHandlerRegistry> eventRegistry;
  std::shared_ptr<OfflineAudioContext> context;
  std::shared_ptr<AudioDestinationNode> destination;

  void SetUp() override {
    eventRegistry = std::make_shared<MockAudioEventHandlerRegistry>();
    context = std::make_shared<OfflineAudioContext>(2, 5 * kSampleRate, kSampleRate, eventRegistry);
    destination = std::make_shared<AudioDestinationNode>(context);
    context->initialize(destination.get());
  }

  std::shared_ptr<TestableAudioFileSourceNode> createFileSource() {
    AudioFileSourceOptions options;
    options.data = makeTestWavPcm16Mono();
    auto fileSource = std::make_shared<TestableAudioFileSourceNode>(context, options);
    EXPECT_NE(fileSource, nullptr);
    return fileSource;
  }

  static constexpr uint64_t kCallbackId = 42;
};

TEST_F(AudioFileSourceNodeTest, NoCallbackMeansNoDispatchRegardlessOfStarvation) {
  auto fileSource = createFileSource();

  EXPECT_CALL(*eventRegistry, dispatchEventFromAudioThread(testing::_, testing::_, testing::_))
      .Times(0);

  // Well past the debounce threshold, but no listener is registered.
  fileSource->callUpdateBufferingState(false, kThresholdFrames * 2);

  EXPECT_FALSE(fileSource->isBuffering());
}

TEST_F(AudioFileSourceNodeTest, StarvationBelowThresholdDoesNotDispatch) {
  auto fileSource = createFileSource();
  fileSource->assignOnBufferingStateChangeCallbackId(kCallbackId);

  EXPECT_CALL(
      *eventRegistry,
      dispatchEventFromAudioThread(AudioEvent::BUFFERING_STATE_CHANGE, kCallbackId, testing::_))
      .Times(0);

  fileSource->callUpdateBufferingState(false, kThresholdFrames - 1);

  EXPECT_FALSE(fileSource->isBuffering());
}

TEST_F(AudioFileSourceNodeTest, StarvationCrossingThresholdDispatchesTrueExactlyOnce) {
  auto fileSource = createFileSource();
  fileSource->assignOnBufferingStateChangeCallbackId(kCallbackId);

  EXPECT_CALL(
      *eventRegistry,
      dispatchEventFromAudioThread(AudioEvent::BUFFERING_STATE_CHANGE, kCallbackId, testing::_))
      .Times(1)
      .WillOnce(testing::Invoke([](AudioEvent, uint64_t, AudioEventPayload payload) {
        auto *boolPayload = std::get_if<BoolValuePayload>(&payload);
        EXPECT_NE(boolPayload, nullptr);
        EXPECT_TRUE(boolPayload->value);
        return true;
      }));

  // Accumulates across calls, like consecutive starved render quanta would.
  fileSource->callUpdateBufferingState(false, kThresholdFrames / 2);
  fileSource->callUpdateBufferingState(false, kThresholdFrames / 2);
  fileSource->callUpdateBufferingState(false, kThresholdFrames / 2);

  EXPECT_TRUE(fileSource->isBuffering());
}

TEST_F(AudioFileSourceNodeTest, RecoveryDispatchesFalseImmediatelyWithNoDebounce) {
  auto fileSource = createFileSource();
  fileSource->assignOnBufferingStateChangeCallbackId(kCallbackId);

  EXPECT_CALL(
      *eventRegistry,
      dispatchEventFromAudioThread(AudioEvent::BUFFERING_STATE_CHANGE, kCallbackId, testing::_))
      .WillOnce(testing::Return(true))
      .WillOnce(testing::Invoke([](AudioEvent, uint64_t, AudioEventPayload payload) {
        auto *boolPayload = std::get_if<BoolValuePayload>(&payload);
        EXPECT_NE(boolPayload, nullptr);
        EXPECT_FALSE(boolPayload->value);
        return true;
      }));

  fileSource->callUpdateBufferingState(false, kThresholdFrames * 2);
  ASSERT_TRUE(fileSource->isBuffering());

  // A single frame of real data recovers immediately — no symmetric debounce.
  fileSource->callUpdateBufferingState(true, 1);

  EXPECT_FALSE(fileSource->isBuffering());
}

TEST_F(AudioFileSourceNodeTest, HasDataWhileNotBufferingNeverDispatches) {
  auto fileSource = createFileSource();
  fileSource->assignOnBufferingStateChangeCallbackId(kCallbackId);

  EXPECT_CALL(*eventRegistry, dispatchEventFromAudioThread(testing::_, testing::_, testing::_))
      .Times(0);

  for (int i = 0; i < 10; ++i) {
    fileSource->callUpdateBufferingState(true, RENDER_QUANTUM_SIZE);
  }

  EXPECT_FALSE(fileSource->isBuffering());
}

TEST_F(AudioFileSourceNodeTest, IntermittentDataResetsStarvationCounter) {
  auto fileSource = createFileSource();
  fileSource->assignOnBufferingStateChangeCallbackId(kCallbackId);

  EXPECT_CALL(*eventRegistry, dispatchEventFromAudioThread(testing::_, testing::_, testing::_))
      .Times(0);

  // Never accumulates enough consecutive starvation to cross the threshold,
  // because a data quantum resets the counter each time — mirrors ordinary
  // decode-ahead jitter rather than a real stall.
  for (int i = 0; i < 20; ++i) {
    fileSource->callUpdateBufferingState(false, kThresholdFrames - 1);
    fileSource->callUpdateBufferingState(true, RENDER_QUANTUM_SIZE);
  }

  EXPECT_FALSE(fileSource->isBuffering());
}

TEST_F(AudioFileSourceNodeTest, PauseClearsBufferingStateWithoutStaleTrue) {
  auto fileSource = createFileSource();
  fileSource->assignOnBufferingStateChangeCallbackId(kCallbackId);

  EXPECT_CALL(
      *eventRegistry,
      dispatchEventFromAudioThread(AudioEvent::BUFFERING_STATE_CHANGE, kCallbackId, testing::_))
      .Times(2); // one true (starvation), one false (pause() resetting it)

  fileSource->callUpdateBufferingState(false, kThresholdFrames * 2);
  ASSERT_TRUE(fileSource->isBuffering());

  fileSource->pause();

  EXPECT_FALSE(fileSource->isBuffering());
}

// NOLINTEND
