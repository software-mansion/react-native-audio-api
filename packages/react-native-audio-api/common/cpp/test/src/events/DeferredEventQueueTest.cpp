#include <audioapi/events/DeferredEventQueue.hpp>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <test/src/MockAudioEventHandlerRegistry.h>

#include <memory>

using namespace audioapi;

namespace {
constexpr uint64_t SOONER_CALLBACK_ID = 11;
constexpr uint64_t LATER_CALLBACK_ID = 22;
constexpr double SOONER_DUE_TIME = 1.0;
constexpr double LATER_DUE_TIME = 2.0;
} // namespace

class DeferredEventQueueTest : public ::testing::Test {
 protected:
  std::shared_ptr<MockAudioEventHandlerRegistry> registry =
      std::make_shared<MockAudioEventHandlerRegistry>();
  DeferredEventQueue queue{registry};
};

TEST_F(DeferredEventQueueTest, IgnoresEventsWithoutACallback) {
  EXPECT_FALSE(queue.defer(AudioEvent::ENDED, 0, SOONER_DUE_TIME));
  EXPECT_EQ(queue.pendingCount(), 0);
}

TEST_F(DeferredEventQueueTest, HoldsEventsBackUntilTheirDueTime) {
  EXPECT_TRUE(queue.defer(AudioEvent::ENDED, SOONER_CALLBACK_ID, SOONER_DUE_TIME));

  EXPECT_CALL(*registry, dispatchEventFromAudioThread(testing::_, testing::_, testing::_)).Times(0);
  queue.dispatchDue(SOONER_DUE_TIME - 0.001);
  EXPECT_EQ(queue.pendingCount(), 1);
}

TEST_F(DeferredEventQueueTest, DispatchesEventsDueExactlyNow) {
  queue.defer(AudioEvent::ENDED, SOONER_CALLBACK_ID, SOONER_DUE_TIME);

  EXPECT_CALL(
      *registry, dispatchEventFromAudioThread(AudioEvent::ENDED, SOONER_CALLBACK_ID, testing::_))
      .WillOnce(testing::Return(true));
  queue.dispatchDue(SOONER_DUE_TIME);
  EXPECT_EQ(queue.pendingCount(), 0);
}

TEST_F(DeferredEventQueueTest, DispatchesInDueTimeOrderNotInsertionOrder) {
  queue.defer(AudioEvent::ENDED, LATER_CALLBACK_ID, LATER_DUE_TIME);
  queue.defer(AudioEvent::ENDED, SOONER_CALLBACK_ID, SOONER_DUE_TIME);

  testing::Sequence dueTimeOrder;
  EXPECT_CALL(
      *registry, dispatchEventFromAudioThread(AudioEvent::ENDED, SOONER_CALLBACK_ID, testing::_))
      .InSequence(dueTimeOrder)
      .WillOnce(testing::Return(true));
  EXPECT_CALL(
      *registry, dispatchEventFromAudioThread(AudioEvent::ENDED, LATER_CALLBACK_ID, testing::_))
      .InSequence(dueTimeOrder)
      .WillOnce(testing::Return(true));

  queue.dispatchDue(LATER_DUE_TIME);
  EXPECT_EQ(queue.pendingCount(), 0);
}

TEST_F(DeferredEventQueueTest, LeavesNotYetDueEventsQueuedAfterASweep) {
  queue.defer(AudioEvent::ENDED, SOONER_CALLBACK_ID, SOONER_DUE_TIME);
  queue.defer(AudioEvent::ENDED, LATER_CALLBACK_ID, LATER_DUE_TIME);

  EXPECT_CALL(
      *registry, dispatchEventFromAudioThread(AudioEvent::ENDED, SOONER_CALLBACK_ID, testing::_))
      .WillOnce(testing::Return(true));
  queue.dispatchDue(SOONER_DUE_TIME);
  EXPECT_EQ(queue.pendingCount(), 1);

  EXPECT_CALL(
      *registry, dispatchEventFromAudioThread(AudioEvent::ENDED, LATER_CALLBACK_ID, testing::_))
      .WillOnce(testing::Return(true));
  queue.dispatchDue(LATER_DUE_TIME);
  EXPECT_EQ(queue.pendingCount(), 0);
}

TEST_F(DeferredEventQueueTest, DropsEventsOnceFullInsteadOfGrowing) {
  for (size_t i = 0; i < DeferredEventQueue::MAX_PENDING_EVENTS; ++i) {
    EXPECT_TRUE(queue.defer(AudioEvent::ENDED, SOONER_CALLBACK_ID + i, SOONER_DUE_TIME));
  }

  EXPECT_FALSE(queue.defer(AudioEvent::ENDED, LATER_CALLBACK_ID, LATER_DUE_TIME));
  EXPECT_EQ(queue.pendingCount(), DeferredEventQueue::MAX_PENDING_EVENTS);

  // The dropped event never fires; everything accepted before it still does.
  EXPECT_CALL(
      *registry, dispatchEventFromAudioThread(AudioEvent::ENDED, LATER_CALLBACK_ID, testing::_))
      .Times(0);
  EXPECT_CALL(*registry, dispatchEventFromAudioThread(AudioEvent::ENDED, testing::_, testing::_))
      .Times(DeferredEventQueue::MAX_PENDING_EVENTS)
      .WillRepeatedly(testing::Return(true));
  queue.dispatchDue(LATER_DUE_TIME);
  EXPECT_EQ(queue.pendingCount(), 0);
}

TEST_F(DeferredEventQueueTest, ToleratesAMissingRegistry) {
  DeferredEventQueue queueWithoutRegistry{nullptr};
  EXPECT_TRUE(queueWithoutRegistry.defer(AudioEvent::ENDED, SOONER_CALLBACK_ID, SOONER_DUE_TIME));
  queueWithoutRegistry.dispatchDue(SOONER_DUE_TIME);
  EXPECT_EQ(queueWithoutRegistry.pendingCount(), 0);
}
