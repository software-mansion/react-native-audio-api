#pragma once

#include <ReactCommon/CallInvoker.h>
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
      const std::string &eventName,
      const std::shared_ptr<facebook::jsi::Function> &handler) = 0;
  virtual void unregisterHandler(const std::string &eventName, uint64_t listenerId) = 0;

  virtual void invokeHandlerWithEventBody(
      const std::string &eventName,
      const std::unordered_map<std::string, EventValue> &body) = 0;
  virtual void invokeHandlerWithEventBody(
      const std::string &eventName,
      uint64_t listenerId,
      const std::unordered_map<std::string, EventValue> &body) = 0;
};

} // namespace audioapi
