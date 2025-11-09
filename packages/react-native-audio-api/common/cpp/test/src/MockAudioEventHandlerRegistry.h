#pragma once

#include <gmock/gmock.h>
#include <audioapi/events/IAudioEventHandlerRegistry.h>
#include <unordered_map>
#include <string>
#include <memory>

using namespace audioapi;

using EventMap = std::unordered_map<std::string, EventValue>;

class MockAudioEventHandlerRegistry : public IAudioEventHandlerRegistry {
 public:
  MOCK_METHOD(uint64_t, registerHandler,
              (const std::string &eventName, const std::shared_ptr<facebook::jsi::Function> &handler), (override));
  MOCK_METHOD(void, unregisterHandler,
              (const std::string &eventName, uint64_t listenerId), (override));

  MOCK_METHOD2(invokeHandlerWithEventBody, void
              (const std::string &eventName, const EventMap &body));
  MOCK_METHOD3(invokeHandlerWithEventBody, void
              (const std::string &eventName, uint64_t listenerId, const EventMap &body));
};
