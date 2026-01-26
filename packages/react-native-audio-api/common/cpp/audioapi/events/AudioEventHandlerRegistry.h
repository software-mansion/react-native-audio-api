#pragma once

#include <ReactCommon/CallInvoker.h>
#include <audioapi/events/AudioEvent.h>
#include <audioapi/events/IAudioEventHandlerRegistry.h>
#include <jsi/jsi.h>
#include <array>
#include <atomic>
#include <memory>
#include <string>
#include <unordered_map>
#include <variant>

namespace audioapi {
using namespace facebook;

using EventValue =
    std::variant<int, float, double, std::string, bool, std::shared_ptr<jsi::HostObject>>;

class AudioEventHandlerRegistry : public IAudioEventHandlerRegistry {
 public:
  explicit AudioEventHandlerRegistry(
      jsi::Runtime *runtime,
      const std::shared_ptr<react::CallInvoker> &callInvoker);
  ~AudioEventHandlerRegistry() override;

  uint64_t registerHandler(AudioEvent eventName, const std::shared_ptr<jsi::Function> &handler)
      override;
  void unregisterHandler(AudioEvent eventName, uint64_t listenerId) override;

  void invokeHandlerWithEventBody(
      AudioEvent eventName,
      const std::unordered_map<std::string, EventValue> &body) override;
  void invokeHandlerWithEventBody(
      AudioEvent eventName,
      uint64_t listenerId,
      const std::unordered_map<std::string, EventValue> &body) override;

 private:
  std::atomic<uint64_t> listenerIdCounter_{1}; // Atomic counter for listener IDs

  std::shared_ptr<react::CallInvoker> callInvoker_;
  jsi::Runtime *runtime_;
  std::unordered_map<AudioEvent, std::unordered_map<uint64_t, std::shared_ptr<jsi::Function>>>
      eventHandlers_;

  jsi::Object createEventObject(const std::unordered_map<std::string, EventValue> &body);
  jsi::Object createEventObject(
      const std::unordered_map<std::string, EventValue> &body,
      size_t memoryPressure);
};

} // namespace audioapi
