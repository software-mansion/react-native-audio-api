#include <audioapi/core/OfflineAudioContext.h>
#include <audioapi/core/destinations/AudioDestinationNode.h>
#include <audioapi/core/sources/AudioScheduledSourceNode.h>
#include <audioapi/core/utils/worklets/SafeIncludes.h>
#include <audioapi/utils/AudioBus.h>
#include <gtest/gtest.h>
#include <test/src/MockAudioEventHandlerRegistry.h>
#include <memory>

using namespace audioapi;
static constexpr int SAMPLE_RATE = 44100;
static constexpr int RENDER_QUANTUM = 128;
static constexpr double RENDER_QUANTUM_TIME = static_cast<double>(RENDER_QUANTUM) / SAMPLE_RATE;

class AudioScheduledSourceTest : public ::testing::Test {
 protected:
  std::shared_ptr<MockAudioEventHandlerRegistry> eventRegistry;
  std::shared_ptr<OfflineAudioContext> context;

  void SetUp() override {
    eventRegistry = std::make_shared<MockAudioEventHandlerRegistry>();
    context = std::make_shared<OfflineAudioContext>(
        2, 5 * SAMPLE_RATE, SAMPLE_RATE, eventRegistry, RuntimeRegistry{});
    context->initialize();
  }
};

class TestableAudioScheduledSourceNode : public AudioScheduledSourceNode {
 public:
  explicit TestableAudioScheduledSourceNode(std::shared_ptr<BaseAudioContext> context)
      : AudioScheduledSourceNode(context) {
    isInitialized_ = true;
  }

  void updatePlaybackInfo(
      const std::shared_ptr<AudioBus> &processingBus,
      int framesToProcess,
      size_t &startOffset,
      size_t &nonSilentFramesToProcess,
      float sampleRate,
      size_t currentSampleFrame) {
    AudioScheduledSourceNode::updatePlaybackInfo(
        processingBus,
        framesToProcess,
        startOffset,
        nonSilentFramesToProcess,
        sampleRate,
        currentSampleFrame);
  }

  std::shared_ptr<AudioBus> processNode(const std::shared_ptr<AudioBus> &, int) override {
    return nullptr;
  }

  PlaybackState getPlaybackState() const {
    return playbackState_;
  }

  void playFrames(int frames) {
    if (std::shared_ptr<BaseAudioContext> context = context_.lock()) {
      size_t startOffset = 0;
      size_t nonSilentFramesToProcess = 0;
      auto processingBus = std::make_shared<AudioBus>(128, 2, static_cast<float>(SAMPLE_RATE));
      updatePlaybackInfo(
          processingBus,
          frames,
          startOffset,
          nonSilentFramesToProcess,
          context->getSampleRate(),
          context->getCurrentSampleFrame());
      context->getDestination()->renderAudio(processingBus, frames);
    }
  }
};

TEST_F(AudioScheduledSourceTest, IsUnscheduledStateSetCorrectly) {
  auto sourceNode = TestableAudioScheduledSourceNode(context);
  EXPECT_EQ(sourceNode.getPlaybackState(), AudioScheduledSourceNode::PlaybackState::UNSCHEDULED);

  sourceNode.start(RENDER_QUANTUM_TIME);
  EXPECT_NE(sourceNode.getPlaybackState(), AudioScheduledSourceNode::PlaybackState::UNSCHEDULED);
}

TEST_F(AudioScheduledSourceTest, IsScheduledStateSetCorrectly) {
  auto sourceNode = TestableAudioScheduledSourceNode(context);
  sourceNode.start(RENDER_QUANTUM_TIME);
  EXPECT_EQ(sourceNode.getPlaybackState(), AudioScheduledSourceNode::PlaybackState::SCHEDULED);

  sourceNode.playFrames(RENDER_QUANTUM);
  EXPECT_EQ(sourceNode.getPlaybackState(), AudioScheduledSourceNode::PlaybackState::SCHEDULED);

  sourceNode.playFrames(1);
  EXPECT_NE(sourceNode.getPlaybackState(), AudioScheduledSourceNode::PlaybackState::SCHEDULED);
}

TEST_F(AudioScheduledSourceTest, IsPlayingStateSetCorrectly) {
  auto sourceNode = TestableAudioScheduledSourceNode(context);
  sourceNode.start(0);
  sourceNode.stop(RENDER_QUANTUM_TIME);

  sourceNode.playFrames(RENDER_QUANTUM);
  EXPECT_EQ(sourceNode.getPlaybackState(), AudioScheduledSourceNode::PlaybackState::PLAYING);

  sourceNode.playFrames(1);
  EXPECT_NE(sourceNode.getPlaybackState(), AudioScheduledSourceNode::PlaybackState::PLAYING);
}

TEST_F(AudioScheduledSourceTest, IsStopScheduledStateSetCorrectly) {
  auto sourceNode = TestableAudioScheduledSourceNode(context);
  sourceNode.start(0);
  sourceNode.stop(RENDER_QUANTUM_TIME);
  sourceNode.playFrames(1); // start playing
  sourceNode.playFrames(RENDER_QUANTUM);
  EXPECT_EQ(sourceNode.getPlaybackState(), AudioScheduledSourceNode::PlaybackState::STOP_SCHEDULED);

  sourceNode.playFrames(1);
  EXPECT_NE(sourceNode.getPlaybackState(), AudioScheduledSourceNode::PlaybackState::STOP_SCHEDULED);
}

TEST_F(AudioScheduledSourceTest, IsFinishedStateSetCorrectly) {
  auto sourceNode = TestableAudioScheduledSourceNode(context);
  sourceNode.start(0);
  sourceNode.stop(RENDER_QUANTUM_TIME);
  sourceNode.playFrames(1); // start playing

  sourceNode.playFrames(RENDER_QUANTUM);
  sourceNode.playFrames(1);
  EXPECT_TRUE(sourceNode.isFinished());
}
