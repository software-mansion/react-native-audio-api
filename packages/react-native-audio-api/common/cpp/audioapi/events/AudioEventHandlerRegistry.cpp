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

void AudioEventHandlerRegistry::invokeHandlerWithEventBody(
    const std::string &eventName,
    const std::unordered_map<std::string, Value> &body) {
  auto it = eventHandlers_.find(eventName);
  if (it != eventHandlers_.end()) {
    for (const auto &pair : it->second) {
      auto handler = pair.second;
      if (handler) {
        callInvoker_->invokeAsync([this, handler, body]() {
          auto eventObject = createEventObject(body);
          handler->call(*runtime_, eventObject);
        });
      }
    }
  }
}

void AudioEventHandlerRegistry::invokeHandlerWithEventBody(
    const std::string &eventName,
    uint64_t listenerId,
    const std::unordered_map<std::string, Value> &body) {
  auto it = eventHandlers_.find(eventName);
  if (it != eventHandlers_.end()) {
    auto handlersMap = it->second;
    auto handlerIt = handlersMap.find(listenerId);
    if (handlerIt != handlersMap.end()) {
      auto handler = handlerIt->second;
      if (handler) {
        callInvoker_->invokeAsync([this, handler, body]() {
          auto eventObject = createEventObject(body);
          handler->call(*runtime_, eventObject);
        });
      }
    }
  }
}

jsi::Object AudioEventHandlerRegistry::createEventObject(
    const std::unordered_map<std::string, Value> &body) {
  auto eventObject = jsi::Object(*runtime_);

  for (const auto &pair : body) {
    const auto name = pair.first.data();
    const auto &value = pair.second;

    if (holds_alternative<int>(value)) {
      eventObject.setProperty(*runtime_, name, get<int>(value));
    } else if (holds_alternative<double>(value)) {
      eventObject.setProperty(*runtime_, name, get<double>(value));
    } else if (holds_alternative<float>(value)) {
      eventObject.setProperty(*runtime_, name, get<float>(value));
    } else if (holds_alternative<bool>(value)) {
      eventObject.setProperty(*runtime_, name, get<bool>(value));
    } else if (holds_alternative<std::string>(value)) {
      eventObject.setProperty(*runtime_, name, get<std::string>(value));
    }
  }

  return eventObject;
}

} // namespace audioapi
