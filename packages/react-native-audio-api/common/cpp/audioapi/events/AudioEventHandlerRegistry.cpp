#pragma once

#include <audioapi/events/AudioEventHandlerRegistry.h>

namespace audioapi {

AudioEventHandlerRegistry::AudioEventHandlerRegistry(
    jsi::Runtime *runtime,
    const std::shared_ptr<react::CallInvoker> &callInvoker) {
  runtime_ = runtime;
  callInvoker_ = callInvoker;

  eventNames_ = {"ended", "remotePlay"};

  for (const auto &eventName : eventNames_) {
    eventHandlers_[eventName] = {};
  }
}

AudioEventHandlerRegistry::~AudioEventHandlerRegistry() {
  eventNames_.clear();
  eventHandlers_.clear();
}

void AudioEventHandlerRegistry::registerHandler(
    const std::string &eventName,
    uint64_t listenerId,
    const std::shared_ptr<jsi::Function> &handler) {
  eventHandlers_[eventName][listenerId] = handler;
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
  std::vector<uint8_t> jsonData(jsonString.begin(), jsonString.end());
  auto it = eventHandlers_.find(eventName);
  if (it != eventHandlers_.end()) {
    for (const auto &pair : it->second) {
      auto handler = pair.second;
      if (handler) {
        callInvoker_->invokeAsync([this, handler, jsonData]() {
          auto result = jsi::Value::createFromJsonUtf8(
              *runtime_, jsonData.data(), jsonData.size());
          handler->call(*runtime_, result);
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
} // namespace audioapi
