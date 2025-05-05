#pragma once

#include <audioapi/AudioEventHandlerRegistry.h>
#include <audioapi/HostObjects/AudioEventHandlerRegistryHostObject.h>

namespace audioapi {

AudioEventHandlerRegistry::AudioEventHandlerRegistry(
    jsi::Runtime *runtime,
    const std::shared_ptr<react::CallInvoker> &callInvoker) {
  eventHandlerRegistry_ = std::make_shared<AudioEventHandlerRegistryHostObject>(
      runtime, callInvoker);
}

template <typename... Args>
void AudioEventHandlerRegistry::invokeHandler(
    const std::string &eventName,
    Args &&...args) {
  eventHandlerRegistry_->invokeHandler(eventName, std::forward<Args>(args)...);
}
} // namespace audioapi
