#pragma once

#include <audioapi/events/AudioEvent.h>
#include <audioapi/events/IAudioEventHandlerRegistry.h>
#include <gmock/gmock.h>
#include <memory>
#include <string>
#include <unordered_map>

using namespace audioapi;

using EventMap = std::unordered_map<std::string, EventValue>;

class MockAudioEventHandlerRegistry : public IAudioEventHandlerRegistry {
 public:
  MOCK_METHOD(
      uint64_t,
      registerHandler,
      (AudioEvent eventName, const std::shared_ptr<facebook::jsi::Function> &handler),
      (override));
  MOCK_METHOD(void, unregisterHandler, (AudioEvent eventName, uint64_t listenerId), (override));

  MOCK_METHOD2(invokeHandlerWithEventBody, void(AudioEvent eventName, const EventMap &body));
  MOCK_METHOD3(
      invokeHandlerWithEventBody,
      void(AudioEvent eventName, uint64_t listenerId, const EventMap &body));
};
