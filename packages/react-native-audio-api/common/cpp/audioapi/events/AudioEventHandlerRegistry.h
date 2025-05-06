#pragma once

#include <jsi/jsi.h>
#include <ReactCommon/CallInvoker.h>
#include <memory>
#include <unordered_map>
#include <array>
#include <string>

namespace audioapi {
using namespace facebook;

class AudioEventHandlerRegistry {
 public:
  explicit AudioEventHandlerRegistry(
      jsi::Runtime *runtime,
      const std::shared_ptr<react::CallInvoker> &callInvoker);
  ~AudioEventHandlerRegistry();

  uint64_t registerHandler(const std::string &eventName, const std::shared_ptr<jsi::Function> &handler);
  void unregisterHandler(const std::string &eventName, uint64_t listenerId);

  template<typename... Args>
  void invokeHandler(const std::string &eventName, uint64_t listenerId, Args&&... args);

  void invokeHandlerWithJsonString(const std::string &eventName, const std::string &jsonString);

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

    static constexpr std::array<std::string_view, 15> AUDIO_API_EVENT_NAMES = {
      "ended",
      "recorderAudioReady",
      "error",
      "systemStateChanged"
    };

    jsi::Value valueFromJsonString(const std::string &jsonString);
};

} // namespace audioapi
