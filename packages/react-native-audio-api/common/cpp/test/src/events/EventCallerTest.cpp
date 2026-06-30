#include <audioapi/events/AudioEventPayloadMapping.h>
#include <audioapi/events/EventCaller.hpp>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <test/src/MockAudioEventHandlerRegistry.h>

#include <memory>

using namespace audioapi;

namespace {
constexpr uint64_t FIRST_CALLBACK_ID = 123;
constexpr uint64_t ENDED_CALLBACK_ID = 77;
constexpr uint64_t ERROR_CALLBACK_ID = 88;
constexpr uint64_t POSITION_CALLBACK_ID = 19;

static_assert(EventPayloadFor<AudioEvent::ENDED, EmptyPayload>);
static_assert(EventPayloadFor<AudioEvent::POSITION_CHANGED, DoubleValuePayload>);
static_assert(EventPayloadFor<AudioEvent::RECORDER_ERROR, StringPayload>);
static_assert(!EventPayloadFor<AudioEvent::ENDED, StringPayload>);
static_assert(!EventPayloadFor<AudioEvent::RECORDER_ERROR, EmptyPayload>);
} // namespace

TEST(EventCallerTest, AssignAndGetCallbackId) {
  auto registry = std::make_shared<MockAudioEventHandlerRegistry>();
  EventCaller<AudioEvent::ENDED> eventCaller(registry);

  eventCaller.assignCallbackId(FIRST_CALLBACK_ID);

  EXPECT_EQ(eventCaller.getCallbackId(), FIRST_CALLBACK_ID);
  EXPECT_TRUE(eventCaller.hasCallback());
}

TEST(EventCallerTest, DispatchEmptySkipsWithoutCallback) {
  auto registry = std::make_shared<MockAudioEventHandlerRegistry>();
  EventCaller<AudioEvent::ENDED> eventCaller(registry);

  EXPECT_CALL(*registry, dispatchEvent(testing::_, testing::_, testing::_)).Times(0);

  EXPECT_FALSE(eventCaller.dispatchEmpty());
}

TEST(EventCallerTest, DispatchEmptyForwardsEventAndCallbackId) {
  auto registry = std::make_shared<MockAudioEventHandlerRegistry>();
  EventCaller<AudioEvent::ENDED> eventCaller(registry);
  eventCaller.assignCallbackId(ENDED_CALLBACK_ID);

  EXPECT_CALL(*registry, dispatchEvent(AudioEvent::ENDED, ENDED_CALLBACK_ID, testing::_))
      .WillOnce(testing::Return(true));

  EXPECT_TRUE(eventCaller.dispatchEmpty());
}

TEST(EventCallerTest, DispatchForwardsPayload) {
  auto registry = std::make_shared<MockAudioEventHandlerRegistry>();
  EventCaller<AudioEvent::RECORDER_ERROR> eventCaller(registry);
  eventCaller.assignCallbackId(ERROR_CALLBACK_ID);

  EXPECT_CALL(*registry, dispatchEvent(AudioEvent::RECORDER_ERROR, ERROR_CALLBACK_ID, testing::_))
      .WillOnce(testing::Invoke([](AudioEvent, uint64_t, AudioEventPayload payload) {
        auto *stringPayload = std::get_if<StringPayload>(&payload);
        EXPECT_NE(stringPayload, nullptr);
        EXPECT_EQ(stringPayload->name, "message");
        EXPECT_EQ(stringPayload->reason, "failed");
        return true;
      }));

  EXPECT_TRUE(eventCaller.dispatch(StringPayload{.name = "message", .reason = "failed"}));
}

TEST(EventCallerTest, UnregisterForwardsEventAndCallbackId) {
  auto registry = std::make_shared<MockAudioEventHandlerRegistry>();
  EventCaller<AudioEvent::POSITION_CHANGED> eventCaller(registry);

  EXPECT_CALL(*registry, unregisterHandler(AudioEvent::POSITION_CHANGED, POSITION_CALLBACK_ID))
      .Times(1);

  eventCaller.unregisterCallback(POSITION_CALLBACK_ID);
}

TEST(EventCallerTest, AssignCallbackIdUnregistersPreviousCallback) {
  auto registry = std::make_shared<MockAudioEventHandlerRegistry>();

  testing::InSequence sequence;
  EXPECT_CALL(*registry, unregisterHandler(AudioEvent::ENDED, FIRST_CALLBACK_ID)).Times(1);
  EXPECT_CALL(*registry, unregisterHandler(AudioEvent::ENDED, ENDED_CALLBACK_ID)).Times(1);

  EventCaller<AudioEvent::ENDED> eventCaller(registry);
  eventCaller.assignCallbackId(FIRST_CALLBACK_ID);
  eventCaller.assignCallbackId(ENDED_CALLBACK_ID);
  EXPECT_EQ(eventCaller.getCallbackId(), ENDED_CALLBACK_ID);
}

TEST(EventCallerTest, DestructorUnregistersAssignedCallback) {
  auto registry = std::make_shared<MockAudioEventHandlerRegistry>();

  EXPECT_CALL(*registry, unregisterHandler(AudioEvent::POSITION_CHANGED, POSITION_CALLBACK_ID))
      .Times(1);

  {
    EventCaller<AudioEvent::POSITION_CHANGED> eventCaller(registry);
    eventCaller.assignCallbackId(POSITION_CALLBACK_ID);
  }
}
