#pragma once

#include <ReactCommon/CallInvoker.h>
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

  uint64_t registerHandler(
      const std::string &eventName,
      const std::shared_ptr<jsi::Function> &handler) override;
  void unregisterHandler(const std::string &eventName, uint64_t listenerId) override;

  void invokeHandlerWithEventBody(
      const std::string &eventName,
      const std::unordered_map<std::string, EventValue> &body) override;
  void invokeHandlerWithEventBody(
      const std::string &eventName,
      uint64_t listenerId,
      const std::unordered_map<std::string, EventValue> &body) override;

 private:
  std::atomic<uint64_t> listenerIdCounter_{1}; // Atomic counter for listener IDs

  std::shared_ptr<react::CallInvoker> callInvoker_;
  jsi::Runtime *runtime_;
  std::unordered_map<std::string, std::unordered_map<uint64_t, std::shared_ptr<jsi::Function>>>
      eventHandlers_;

  static constexpr std::array<std::string_view, 15> SYSTEM_EVENT_NAMES = {
      "remotePlay",
      "remotePause",
      "remoteStop",
      "remoteTogglePlayPause",
      "remoteChangePlaybackRate",
      "remoteNextTrack",
      "remotePreviousTrack",
      "remoteSkipForward",
      "remoteSkipBackward",
      "remoteSeekForward",
      "remoteSeekBackward",
      "remoteChangePlaybackPosition",
      "routeChange",
      "interruption",
      "volumeChange",
  };

  static constexpr std::array<std::string_view, 6> AUDIO_API_EVENT_NAMES =
      {"ended", "loopEnded", "audioReady", "positionChanged", "audioError", "systemStateChanged"};

  jsi::Object createEventObject(const std::unordered_map<std::string, EventValue> &body);
  jsi::Object createEventObject(
      const std::unordered_map<std::string, EventValue> &body,
      size_t memoryPressure);
};

} // namespace audioapi
