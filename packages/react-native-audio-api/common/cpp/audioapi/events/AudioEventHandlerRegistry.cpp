#pragma once

#include <audioapi/events/AudioEventHandlerRegistry.h>

namespace audioapi {

AudioEventHandlerRegistry::AudioEventHandlerRegistry(
    jsi::Runtime *runtime,
    const std::shared_ptr<react::CallInvoker> &callInvoker) {
    runtime_ = runtime;
    callInvoker_ = callInvoker;

    eventNames_ = {"ended"};

    for (const auto &eventName : eventNames_) {
        eventHandlers_[eventName] = {};
    }
}

AudioEventHandlerRegistry::~AudioEventHandlerRegistry() {
  eventNames_.clear();
  eventHandlers_.clear();
}

void AudioEventHandlerRegistry::registerHandler(const std::string &eventName, uint64_t listenerId,
                                               const jsi::Value &value) {
    eventHandlers_[eventName][listenerId] = std::make_shared<jsi::Function>(value.getObject(*runtime_).getFunction(*runtime_));
}

void AudioEventHandlerRegistry::unregisterHandler(const std::string &eventName, uint64_t listenerId) {
    auto it = eventHandlers_.find(eventName);
    if (it != eventHandlers_.end()) {
        it->second.erase(listenerId);
    }
}

template <typename... Args>
void AudioEventHandlerRegistry::invokeHandler(
    const std::string &eventName,
    Args &&...args) {
    auto it = eventHandlers_.find(eventName);
    if (it != eventHandlers_.end()) {
        for (const auto &pair : it->second) {
            auto handler = pair.second;
            if (handler) {
                callInvoker_->invokeAsync([this, handler, args...]() {
                    handler->call(*runtime_, args...);
                });
            }
        }
    }
}

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
                callInvoker_->invokeAsync([this, handler, args...]() {
                    handler->call(*runtime_, args...);
                });
            }
        }
    }
}
} // namespace audioapi
