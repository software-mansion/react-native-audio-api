#pragma once

#include <ReactCommon/CallInvoker.h>
#include <audioapi/events/AudioEvent.h>
#include <jsi/jsi.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <variant>

namespace audioapi {

using EventValue =
    std::variant<int, float, double, std::string, bool, std::shared_ptr<facebook::jsi::HostObject>>;

class IAudioEventHandlerRegistry {
 public:
  virtual ~IAudioEventHandlerRegistry() = default;

  virtual uint64_t registerHandler(
      AudioEvent eventName,
      const std::shared_ptr<facebook::jsi::Function> &handler) = 0;
  virtual void unregisterHandler(AudioEvent eventName, uint64_t listenerId) = 0;

  virtual void invokeHandlerWithEventBody(
      AudioEvent eventName,
      const std::unordered_map<std::string, EventValue> &body) = 0;
  virtual void invokeHandlerWithEventBody(
      AudioEvent eventName,
      uint64_t listenerId,
      const std::unordered_map<std::string, EventValue> &body) = 0;
};

} // namespace audioapi
