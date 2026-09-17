#include <audioapi/core/OfflineAudioContext.h>
#include <audioapi/core/destinations/AudioDestinationNode.h>
#include <audioapi/core/sources/AudioScheduledSourceNode.h>
#include <audioapi/events/AudioEvent.h>
#include <audioapi/utils/AudioBuffer.hpp>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <test/src/MockAudioEventHandlerRegistry.h>
#include <memory>

using namespace audioapi;

// NOLINTBEGIN

static constexpr int SAMPLE_RATE = 44100;
static constexpr int RENDER_QUANTUM = 128;
static constexpr double RENDER_QUANTUM_TIME = static_cast<double>(RENDER_QUANTUM) / SAMPLE_RATE;

class AudioScheduledSourceTest : public ::testing::Test {
 protected:
  std::shared_ptr<MockAudioEventHandlerRegistry> eventRegistry;
  std::shared_ptr<OfflineAudioContext> context;
  std::shared_ptr<AudioDestinationNode> destination;
  static constexpr int sampleRate = 44100;

  void SetUp() override {
    eventRegistry = std::make_shared<MockAudioEventHandlerRegistry>();
    context = std::make_shared<OfflineAudioContext>(2, 5 * sampleRate, sampleRate, eventRegistry);
    destination = std::make_shared<AudioDestinationNode>(context);
    context->initialize(destination.get());
  }
};

class TestableAudioScheduledSourceNode : public AudioScheduledSourceNode {
 public:
  explicit TestableAudioScheduledSourceNode(std::shared_ptr<BaseAudioContext> context)
      : AudioScheduledSourceNode(context) {}

  void updatePlaybackInfo(
      const std::shared_ptr<DSPAudioBuffer> &processingBuffer,
      int framesToProcess,
      size_t &startOffset,
      size_t &nonSilentFramesToProcess,
      float sampleRate,
      size_t currentSampleFrame) {
    AudioScheduledSourceNode::updatePlaybackInfo(
        processingBuffer,
        framesToProcess,
        startOffset,
        nonSilentFramesToProcess,
        sampleRate,
        currentSampleFrame);
  }

  void processNode(int) override {}

  PlaybackState getPlaybackState() const {
    return playbackState_;
  }

  void playFrames(int frames) {
    if (std::shared_ptr<BaseAudioContext> context = context_.lock()) {
      size_t startOffset = 0;
      size_t nonSilentFramesToProcess = 0;
      auto processingBuffer =
          std::make_shared<DSPAudioBuffer>(128, 2, static_cast<float>(SAMPLE_RATE));
      updatePlaybackInfo(
          processingBuffer,
          frames,
          startOffset,
          nonSilentFramesToProcess,
          context->getSampleRate(),
          context->getCurrentSampleFrame());
      context->processGraph(processingBuffer.get(), frames);
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

TEST_F(AudioScheduledSourceTest, StopBeforeStartFiresEndedWhenContextTimeReachesStopTime) {
  static constexpr uint64_t ENDED_CALLBACK_ID = 42;
  auto sourceNode = TestableAudioScheduledSourceNode(context);
  sourceNode.assignOnEndedCallbackId(ENDED_CALLBACK_ID);

  sourceNode.start(2 * RENDER_QUANTUM_TIME);
  sourceNode.stop(RENDER_QUANTUM_TIME);
  EXPECT_TRUE(sourceNode.isFinished());

  EXPECT_CALL(
      *eventRegistry,
      dispatchEventFromAudioThread(AudioEvent::ENDED, ENDED_CALLBACK_ID, testing::_))
      .Times(0);
  sourceNode.playFrames(RENDER_QUANTUM); // context time is still before the stop time

  EXPECT_CALL(
      *eventRegistry,
      dispatchEventFromAudioThread(AudioEvent::ENDED, ENDED_CALLBACK_ID, testing::_))
      .WillOnce(testing::Return(true));
  sourceNode.playFrames(RENDER_QUANTUM); // context time reaches the stop time

  EXPECT_CALL(*eventRegistry, unregisterHandler(AudioEvent::ENDED, ENDED_CALLBACK_ID)).Times(1);
}

TEST_F(AudioScheduledSourceTest, DeferredEndedEventsFireInDueTimeOrderNotInsertionOrder) {
  static constexpr uint64_t LATE_CALLBACK_ID = 43;
  static constexpr uint64_t EARLY_CALLBACK_ID = 44;

  auto lateNode = TestableAudioScheduledSourceNode(context);
  lateNode.assignOnEndedCallbackId(LATE_CALLBACK_ID);
  lateNode.start(3 * RENDER_QUANTUM_TIME);
  lateNode.stop(2 * RENDER_QUANTUM_TIME);

  // Deferred after the later one, so insertion order is the reverse of due-time order.
  auto earlyNode = TestableAudioScheduledSourceNode(context);
  earlyNode.assignOnEndedCallbackId(EARLY_CALLBACK_ID);
  earlyNode.start(2 * RENDER_QUANTUM_TIME);
  earlyNode.stop(RENDER_QUANTUM_TIME);

  EXPECT_CALL(
      *eventRegistry, dispatchEventFromAudioThread(AudioEvent::ENDED, testing::_, testing::_))
      .Times(0);
  lateNode.playFrames(RENDER_QUANTUM); // context time is still 0

  EXPECT_CALL(
      *eventRegistry, dispatchEventFromAudioThread(AudioEvent::ENDED, LATE_CALLBACK_ID, testing::_))
      .Times(0);
  EXPECT_CALL(
      *eventRegistry,
      dispatchEventFromAudioThread(AudioEvent::ENDED, EARLY_CALLBACK_ID, testing::_))
      .WillOnce(testing::Return(true));
  lateNode.playFrames(RENDER_QUANTUM); // context time reaches the earlier stop time

  EXPECT_CALL(
      *eventRegistry, dispatchEventFromAudioThread(AudioEvent::ENDED, LATE_CALLBACK_ID, testing::_))
      .WillOnce(testing::Return(true));
  lateNode.playFrames(RENDER_QUANTUM); // context time reaches the later stop time

  EXPECT_CALL(*eventRegistry, unregisterHandler(AudioEvent::ENDED, testing::_))
      .Times(testing::AnyNumber());
}

TEST_F(AudioScheduledSourceTest, DeferredEndedEventsDueInTheSameQuantumFireInDueTimeOrder) {
  static constexpr uint64_t LATER_CALLBACK_ID = 45;
  static constexpr uint64_t SOONER_CALLBACK_ID = 46;

  // Both due times land inside the first rendered quantum, so a single sweep
  // dispatches both and the sweep order is observable.
  auto laterNode = TestableAudioScheduledSourceNode(context);
  laterNode.assignOnEndedCallbackId(LATER_CALLBACK_ID);
  laterNode.start(0.75 * RENDER_QUANTUM_TIME);
  laterNode.stop(0.5 * RENDER_QUANTUM_TIME);

  auto soonerNode = TestableAudioScheduledSourceNode(context);
  soonerNode.assignOnEndedCallbackId(SOONER_CALLBACK_ID);
  soonerNode.start(0.5 * RENDER_QUANTUM_TIME);
  soonerNode.stop(0.25 * RENDER_QUANTUM_TIME);

  laterNode.playFrames(RENDER_QUANTUM); // context time is still 0, nothing is due

  testing::Sequence dueTimeOrder;
  EXPECT_CALL(
      *eventRegistry,
      dispatchEventFromAudioThread(AudioEvent::ENDED, SOONER_CALLBACK_ID, testing::_))
      .InSequence(dueTimeOrder)
      .WillOnce(testing::Return(true));
  EXPECT_CALL(
      *eventRegistry,
      dispatchEventFromAudioThread(AudioEvent::ENDED, LATER_CALLBACK_ID, testing::_))
      .InSequence(dueTimeOrder)
      .WillOnce(testing::Return(true));
  laterNode.playFrames(RENDER_QUANTUM); // both due times have passed

  EXPECT_CALL(*eventRegistry, unregisterHandler(AudioEvent::ENDED, testing::_))
      .Times(testing::AnyNumber());
}

// NOLINTEND
