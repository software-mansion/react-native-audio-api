#include <audioapi/events/AudioEventPayload.h>
#include <audioapi/utils/events/BufferingStateDispatcher.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <test/src/MockAudioEventHandlerRegistry.h>

#include <memory>

using namespace audioapi;

// NOLINTBEGIN

namespace {
constexpr uint64_t kCallbackId = 42;
constexpr int kThresholdFrames = 6615; // 150ms @ 44100Hz, matching production usage
} // namespace

TEST(BufferingStateDispatcherTest, NoCallbackMeansNoDispatchRegardlessOfStarvation) {
  auto registry = std::make_shared<MockAudioEventHandlerRegistry>();
  BufferingStateDispatcher dispatcher(registry, kThresholdFrames);

  EXPECT_CALL(*registry, dispatchEventFromAudioThread(testing::_, testing::_, testing::_)).Times(0);

  // Well past the debounce threshold, but no listener is registered.
  dispatcher.advance(/* hasData */ false, kThresholdFrames * 2);

  EXPECT_FALSE(dispatcher.isBuffering());
}

TEST(BufferingStateDispatcherTest, StarvationBelowThresholdDoesNotDispatch) {
  auto registry = std::make_shared<MockAudioEventHandlerRegistry>();
  BufferingStateDispatcher dispatcher(registry, kThresholdFrames);
  dispatcher.assignCallbackId(kCallbackId);

  EXPECT_CALL(
      *registry,
      dispatchEventFromAudioThread(AudioEvent::BUFFERING_STATE_CHANGE, kCallbackId, testing::_))
      .Times(0);

  dispatcher.advance(/* hasData */ false, kThresholdFrames - 1);

  EXPECT_FALSE(dispatcher.isBuffering());
}

TEST(BufferingStateDispatcherTest, StarvationCrossingThresholdDispatchesTrueExactlyOnce) {
  auto registry = std::make_shared<MockAudioEventHandlerRegistry>();
  BufferingStateDispatcher dispatcher(registry, kThresholdFrames);
  dispatcher.assignCallbackId(kCallbackId);

  EXPECT_CALL(
      *registry,
      dispatchEventFromAudioThread(AudioEvent::BUFFERING_STATE_CHANGE, kCallbackId, testing::_))
      .Times(1)
      .WillOnce(testing::Invoke([](AudioEvent, uint64_t, AudioEventPayload payload) {
        auto *boolPayload = std::get_if<BoolValuePayload>(&payload);
        EXPECT_NE(boolPayload, nullptr);
        EXPECT_TRUE(boolPayload->value);
        return true;
      }));

  // Accumulates across calls, like consecutive starved render quanta would.
  dispatcher.advance(false, kThresholdFrames / 2);
  dispatcher.advance(false, kThresholdFrames / 2);
  dispatcher.advance(false, kThresholdFrames / 2);

  EXPECT_TRUE(dispatcher.isBuffering());
}

TEST(BufferingStateDispatcherTest, RecoveryDispatchesFalseImmediatelyWithNoDebounce) {
  auto registry = std::make_shared<MockAudioEventHandlerRegistry>();
  BufferingStateDispatcher dispatcher(registry, kThresholdFrames);
  dispatcher.assignCallbackId(kCallbackId);

  EXPECT_CALL(
      *registry,
      dispatchEventFromAudioThread(AudioEvent::BUFFERING_STATE_CHANGE, kCallbackId, testing::_))
      .WillOnce(testing::Return(true))
      .WillOnce(testing::Invoke([](AudioEvent, uint64_t, AudioEventPayload payload) {
        auto *boolPayload = std::get_if<BoolValuePayload>(&payload);
        EXPECT_NE(boolPayload, nullptr);
        EXPECT_FALSE(boolPayload->value);
        return true;
      }));

  dispatcher.advance(false, kThresholdFrames * 2);
  ASSERT_TRUE(dispatcher.isBuffering());

  // A single frame of real data recovers immediately — no symmetric debounce.
  dispatcher.advance(true, 1);

  EXPECT_FALSE(dispatcher.isBuffering());
}

TEST(BufferingStateDispatcherTest, HasDataWhileNotBufferingNeverDispatches) {
  auto registry = std::make_shared<MockAudioEventHandlerRegistry>();
  BufferingStateDispatcher dispatcher(registry, kThresholdFrames);
  dispatcher.assignCallbackId(kCallbackId);

  EXPECT_CALL(*registry, dispatchEventFromAudioThread(testing::_, testing::_, testing::_)).Times(0);

  for (int i = 0; i < 10; ++i) {
    dispatcher.advance(true, 128);
  }

  EXPECT_FALSE(dispatcher.isBuffering());
}

TEST(BufferingStateDispatcherTest, IntermittentDataResetsStarvationCounter) {
  auto registry = std::make_shared<MockAudioEventHandlerRegistry>();
  BufferingStateDispatcher dispatcher(registry, kThresholdFrames);
  dispatcher.assignCallbackId(kCallbackId);

  EXPECT_CALL(*registry, dispatchEventFromAudioThread(testing::_, testing::_, testing::_)).Times(0);

  // Never accumulates enough consecutive starvation to cross the threshold,
  // because a data quantum resets the counter each time — mirrors ordinary
  // decode-ahead jitter rather than a real stall.
  for (int i = 0; i < 20; ++i) {
    dispatcher.advance(false, kThresholdFrames - 1);
    dispatcher.advance(true, 128);
  }

  EXPECT_FALSE(dispatcher.isBuffering());
}

TEST(BufferingStateDispatcherTest, AssignCallbackIdUnregistersPreviousCallback) {
  auto registry = std::make_shared<MockAudioEventHandlerRegistry>();
  BufferingStateDispatcher dispatcher(registry, kThresholdFrames);

  testing::InSequence sequence;
  EXPECT_CALL(*registry, unregisterHandler(AudioEvent::BUFFERING_STATE_CHANGE, kCallbackId))
      .Times(1);
  EXPECT_CALL(*registry, unregisterHandler(AudioEvent::BUFFERING_STATE_CHANGE, kCallbackId + 1))
      .Times(1);

  dispatcher.assignCallbackId(kCallbackId);
  dispatcher.assignCallbackId(kCallbackId + 1);
  EXPECT_EQ(dispatcher.getCallbackId(), kCallbackId + 1);
}

// NOLINTEND
