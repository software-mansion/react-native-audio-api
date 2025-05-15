#pragma once

#include <jsi/jsi.h>
#include <ReactCommon/CallInvoker.h>
#include <memory>
#include <unordered_map>
#include <array>
#include <string>
#include <variant>

namespace audioapi {
using namespace facebook;

using EventValue = std::variant<int, float, double, std::string, bool, std::shared_ptr<jsi::HostObject>>;

class AudioEventHandlerRegistry {
 public:
  explicit AudioEventHandlerRegistry(
      jsi::Runtime *runtime,
      const std::shared_ptr<react::CallInvoker> &callInvoker);
  ~AudioEventHandlerRegistry();

  uint64_t registerHandler(const std::string &eventName, const std::shared_ptr<jsi::Function> &handler);
  void unregisterHandler(const std::string &eventName, uint64_t listenerId);

  void invokeHandlerWithEventBody(const std::string &eventName, const std::unordered_map<std::string, EventValue> &body);
  void invokeHandlerWithEventBody(const std::string &eventName, uint64_t listenerId, const std::unordered_map<std::string, EventValue> &body);

 private:
    std::shared_ptr<react::CallInvoker> callInvoker_;
    jsi::Runtime *runtime_;
    std::unordered_map<std::string, std::unordered_map<uint64_t, std::shared_ptr<jsi::Function>>> eventHandlers_;

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

    static constexpr std::array<std::string_view, 4> AUDIO_API_EVENT_NAMES = {
      "ended",
      "audioReady",
      "audioError",
      "systemStateChanged"
    };

    jsi::Object createEventObject(const std::unordered_map<std::string, EventValue> &body);
};

} // namespace audioapi
