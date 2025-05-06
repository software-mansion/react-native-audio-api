#pragma once

#include <audioapi/events/AudioEventHandlerRegistry.h>

namespace audioapi {

AudioEventHandlerRegistry::AudioEventHandlerRegistry(
    jsi::Runtime *runtime,
    const std::shared_ptr<react::CallInvoker> &callInvoker) {
  runtime_ = runtime;
  callInvoker_ = callInvoker;

  for (const auto &eventName : SYSTEM_EVENT_NAMES) {
    eventHandlers_[std::string(eventName)] = {};
  }

  for (const auto &eventName : AUDIO_API_EVENT_NAMES) {
    eventHandlers_[std::string(eventName)] = {};
  }
}

AudioEventHandlerRegistry::~AudioEventHandlerRegistry() {
  eventHandlers_.clear();
}

uint64_t AudioEventHandlerRegistry::registerHandler(
    const std::string &eventName,
    const std::shared_ptr<jsi::Function> &handler) {
  static uint64_t LISTENER_ID = 0;
  LISTENER_ID++;

  eventHandlers_[eventName][LISTENER_ID] = handler;

  return LISTENER_ID;
}

void AudioEventHandlerRegistry::unregisterHandler(
    const std::string &eventName,
    uint64_t listenerId) {
  auto it = eventHandlers_.find(eventName);
  if (it != eventHandlers_.end()) {
    it->second.erase(listenerId);
  }
}

void AudioEventHandlerRegistry::invokeHandlerWithJsonString(
    const std::string &eventName,
    const std::string &jsonString) {
  auto it = eventHandlers_.find(eventName);
  if (it != eventHandlers_.end()) {
    for (const auto &pair : it->second) {
      auto handler = pair.second;
      if (handler) {
        callInvoker_->invokeAsync([this, handler, jsonString]() {
          handler->call(*runtime_, valueFromJsonString(jsonString));
        });
      }
    }
  }
}

// template <typename... Args>
// void AudioEventHandlerRegistry::invokeHandler(
//     const std::string &eventName,
//     Args &&...args) {
//   auto it = eventHandlers_.find(eventName);
//   if (it != eventHandlers_.end()) {
//     for (const auto &pair : it->second) {
//       auto handler = pair.second;
//       if (handler) {
//         callInvoker_->invokeAsync(
//             [this, handler, args...]() { handler->call(*runtime_, args...);
//             });
//       }
//     }
//   }
// }

template <typename... Args>
void AudioEventHandlerRegistry::invokeHandler(
    const std::string &eventName,
    uint64_t listenerId,
    Args &&...args) {
  auto it = eventHandlers_.find(eventName);
  if (it != eventHandlers_.end()) {
    auto handlersMap = it->second;
    auto handlerIt = handlersMap.find(listenerId);
    if (handlerIt != handlersMap.end()) {
      auto handler = handlerIt->second;
      if (handler) {
        callInvoker_->invokeAsync(
            [this, handler, args...]() { handler->call(*runtime_, args...); });
      }
    }
  }
}

jsi::Value AudioEventHandlerRegistry::valueFromJsonString(const std::string &jsonString) {
  std::vector<uint8_t> jsonData(jsonString.begin(), jsonString.end());

  return jsi::Value::createFromJsonUtf8(
      *runtime_, jsonData.data(), jsonData.size());
}
} // namespace audioapi
