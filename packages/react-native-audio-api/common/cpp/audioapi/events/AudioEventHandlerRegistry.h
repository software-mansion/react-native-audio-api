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

/// @brief A registry for audio event handlers.
/// It allows registering, unregistering, and invoking event handlers for audio events.
/// State changes are performed only on the JS thread.
/// State access is thread-safe via the RN CallInvoker.
class AudioEventHandlerRegistry : public IAudioEventHandlerRegistry,
                                  public std::enable_shared_from_this<AudioEventHandlerRegistry> {
 public:
  explicit AudioEventHandlerRegistry(
      jsi::Runtime *runtime,
      const std::shared_ptr<react::CallInvoker> &callInvoker);
  ~AudioEventHandlerRegistry() override;

  /// @brief Registers an event handler for a specific audio event.
  /// @param eventName The name of the audio event.
  /// @param handler The JavaScript function to be called when the event occurs.
  /// @return A unique listener ID for the registered handler.
  /// @note Thread safe
  uint64_t registerHandler(AudioEvent eventName, const std::shared_ptr<jsi::Function> &handler)
      override;

  /// @brief Unregisters an event handler for a specific audio event using its listener ID.
  /// @param eventName The name of the audio event.
  /// @param listenerId The unique listener ID of the handler to be unregistered.
  /// @note Thread safe
  void unregisterHandler(AudioEvent eventName, uint64_t listenerId) override;

  /// @brief Invokes the event handler(s) for a specific event with the provided event body.
  /// @note Thread safe
  void invokeHandlerWithEventBody(
      AudioEvent eventName,
      const std::unordered_map<std::string, EventValue> &body) override;

  /// @brief Invokes the event handler(s) for a specific event with the provided event body.
  /// @note Thread safe
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
