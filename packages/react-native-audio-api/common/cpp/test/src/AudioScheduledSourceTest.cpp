#include <audioapi/core/OfflineAudioContext.h>
#include <audioapi/core/destinations/AudioDestinationNode.h>
#include <audioapi/core/sources/AudioScheduledSourceNode.h>
#include <audioapi/core/utils/worklets/SafeIncludes.h>
#include <audioapi/utils/AudioBus.h>
#include <gtest/gtest.h>
#include <test/src/MockAudioEventHandlerRegistry.h>

using namespace audioapi;
static constexpr int SAMPLE_RATE = 44100;
static constexpr double START_TIME = 0.5;
static constexpr double STOP_TIME = 0.6;

class AudioScheduledSourceTest : public ::testing::Test {
 protected:
  std::shared_ptr<MockAudioEventHandlerRegistry> eventRegistry;
  std::unique_ptr<OfflineAudioContext> context;

  void SetUp() override {
    eventRegistry = std::make_shared<MockAudioEventHandlerRegistry>();
    context = std::make_unique<OfflineAudioContext>(
        2, 5 * SAMPLE_RATE, SAMPLE_RATE, eventRegistry, RuntimeRegistry{});
  }
};

class TestableAudioScheduledSourceNode : public AudioScheduledSourceNode {
 public:
  explicit TestableAudioScheduledSourceNode(BaseAudioContext *context)
      : AudioScheduledSourceNode(context) {
    isInitialized_ = true;
  }

  void updatePlaybackInfo(
      const std::shared_ptr<AudioBus> &processingBus,
      int framesToProcess,
      size_t &startOffset,
      size_t &nonSilentFramesToProcess) {
    AudioScheduledSourceNode::updatePlaybackInfo(
        processingBus, framesToProcess, startOffset, nonSilentFramesToProcess);
  }

  std::shared_ptr<AudioBus> processNode(const std::shared_ptr<AudioBus> &, int)
      override {
    return nullptr;
  }

  PlaybackState getPlaybackState() const {
    return playbackState_;
  }

  void playFrames(int frames) {
    size_t startOffset = 0;
    size_t nonSilentFramesToProcess = 0;
    auto processingBus =
        std::make_shared<AudioBus>(128, 2, static_cast<float>(SAMPLE_RATE));
    updatePlaybackInfo(
        processingBus, frames, startOffset, nonSilentFramesToProcess);
    context_->getDestination()->renderAudio(processingBus, frames);
  }
};

TEST_F(AudioScheduledSourceTest, IsUnscheduledStateSetCorrectly) {
  auto sourceNode = TestableAudioScheduledSourceNode(context.get());
  EXPECT_TRUE(sourceNode.isUnscheduled());

  sourceNode.start(0.5);
  EXPECT_FALSE(sourceNode.isUnscheduled());
}

TEST_F(AudioScheduledSourceTest, IsScheduledStateSetCorrectly) {
  auto sourceNode = TestableAudioScheduledSourceNode(context.get());
  sourceNode.start(START_TIME);
  EXPECT_TRUE(sourceNode.isScheduled());

  sourceNode.playFrames(SAMPLE_RATE * START_TIME - 1);
  EXPECT_TRUE(sourceNode.isScheduled());

  sourceNode.playFrames(1);
  EXPECT_FALSE(sourceNode.isScheduled());
}

TEST_F(AudioScheduledSourceTest, IsPlayingStateSetCorrectly) {
  auto sourceNode = TestableAudioScheduledSourceNode(context.get());
  sourceNode.start(START_TIME);
  sourceNode.stop(START_TIME + 0.1);

  sourceNode.playFrames(SAMPLE_RATE * START_TIME);
  EXPECT_TRUE(sourceNode.isPlaying());

  sourceNode.playFrames(SAMPLE_RATE * 0.1 - 1);
  EXPECT_TRUE(sourceNode.isPlaying());

  sourceNode.playFrames(1);
  EXPECT_FALSE(sourceNode.isPlaying());
}

TEST_F(AudioScheduledSourceTest, IsStopScheduledStateSetCorrectly) {
  auto sourceNode = TestableAudioScheduledSourceNode(context.get());
  sourceNode.start(0);
  sourceNode.stop(STOP_TIME);
  sourceNode.playFrames(1); // start playing

  sourceNode.playFrames(SAMPLE_RATE * STOP_TIME);
  EXPECT_TRUE(sourceNode.isStopScheduled());

  sourceNode.playFrames(1);
  EXPECT_FALSE(sourceNode.isStopScheduled());
}

TEST_F(AudioScheduledSourceTest, IsFinishedStateSetCorrectly) {
  auto sourceNode = TestableAudioScheduledSourceNode(context.get());
  sourceNode.start(0);
  sourceNode.stop(STOP_TIME);
  sourceNode.playFrames(1); // start playing

  EXPECT_CALL(
      *eventRegistry,
      invokeHandlerWithEventBody("ended", testing::_, testing::_))
      .Times(1);
  sourceNode.playFrames(SAMPLE_RATE * STOP_TIME);
  sourceNode.playFrames(1);
  EXPECT_TRUE(sourceNode.isFinished());
}
