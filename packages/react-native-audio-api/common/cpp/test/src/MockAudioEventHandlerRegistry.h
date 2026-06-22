#pragma once

#include <audioapi/events/AudioEvent.h>
#include <audioapi/events/AudioEventPayload.h>
#include <audioapi/events/IAudioEventHandlerRegistry.h>
#include <gmock/gmock.h>
#include <memory>

using namespace audioapi;

class MockAudioEventHandlerRegistry : public IAudioEventHandlerRegistry {
 public:
  MOCK_METHOD(
      uint64_t,
      registerHandler,
      (AudioEvent eventName, const std::shared_ptr<facebook::jsi::Function> &handler),
      (override));
  MOCK_METHOD(void, unregisterHandler, (AudioEvent eventName, uint64_t listenerId), (override));
  MOCK_METHOD(
      bool,
      dispatchEvent,
      (AudioEvent eventName, uint64_t listenerId, AudioEventPayload &&payload),
      (noexcept, override));
  MOCK_METHOD(
      bool,
      dispatchEventFromAudioThread,
      (AudioEvent eventName, uint64_t listenerId, AudioEventPayload &&payload),
      (noexcept, override));
};
