#pragma once

#include <gmock/gmock.h>
#include <audioapi/events/IAudioEventHandlerRegistry.h>
#include <unordered_map>
#include <string>
#include <memory>

using EventMap = std::unordered_map<std::string, audioapi::EventValue>;

class MockAudioEventHandlerRegistry : public audioapi::IAudioEventHandlerRegistry {
 public:
  MOCK_METHOD(uint64_t, registerHandler,
              (const std::string &eventName, const std::shared_ptr<facebook::jsi::Function> &handler), (override));
  MOCK_METHOD(void, unregisterHandler,
              (const std::string &eventName, uint64_t listenerId), (override));

  MOCK_METHOD(void, invokeHandlerWithEventBody,
              (const std::string &eventName, const EventMap &body), (override));
  MOCK_METHOD(void, invokeHandlerWithEventBody,
              (const std::string &eventName, uint64_t listenerId, const EventMap &body), (override));
};
