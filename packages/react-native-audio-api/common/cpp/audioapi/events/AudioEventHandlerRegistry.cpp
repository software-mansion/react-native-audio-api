#include <audioapi/HostObjects/sources/AudioBufferHostObject.h>
#include <audioapi/events/AudioEventHandlerRegistry.h>
#include <cstdio>
#include <memory>
#include <string>
#include <unordered_map>

namespace audioapi {

AudioEventHandlerRegistry::AudioEventHandlerRegistry(
    jsi::Runtime *runtime,
    const std::shared_ptr<react::CallInvoker> &callInvoker)
    : IAudioEventHandlerRegistry(), callInvoker_(callInvoker), runtime_(runtime) {}

AudioEventHandlerRegistry::~AudioEventHandlerRegistry() {
  eventHandlers_.clear();
}

uint64_t AudioEventHandlerRegistry::registerHandler(
    AudioEvent eventName,
    const std::shared_ptr<jsi::Function> &handler) {
  auto listenerId = listenerIdCounter_.fetch_add(1, std::memory_order_relaxed);

  if (runtime_ == nullptr) {
    // If runtime is not valid, we cannot register the handler
    return 0;
  }

  auto weakSelf = weak_from_this();

  // Read/Write on eventHandlers_ map only on the JS thread
  callInvoker_->invokeAsync([weakSelf, eventName, listenerId, handler]() {
    if (auto self = weakSelf.lock()) {
      self->eventHandlers_[eventName][listenerId] = handler;
    }
  });

  return listenerId;
}

void AudioEventHandlerRegistry::unregisterHandler(AudioEvent eventName, uint64_t listenerId) {
  if (runtime_ == nullptr) {
    // If runtime is not valid, we cannot unregister the handler
    return;
  }

  auto weakSelf = weak_from_this();

  // Read/Write on eventHandlers_ map only on the JS thread
  callInvoker_->invokeAsync([weakSelf, eventName, listenerId]() {
    if (auto self = weakSelf.lock()) {
      auto it = self->eventHandlers_.find(eventName);

      if (it == self->eventHandlers_.end()) {
        return;
      }

      auto &handlersMap = it->second;
      auto handlerIt = handlersMap.find(listenerId);

      if (handlerIt != handlersMap.end()) {
        handlersMap.erase(handlerIt);
      }
    }
  });
}

jsi::Object AudioEventHandlerRegistry::makeEventObjectForDispatch(
    AudioEvent eventName,
    const std::unordered_map<std::string, EventValue> &body) {
  if (eventName != AudioEvent::AUDIO_READY) {
    return createEventObject(body);
  }
  auto bufferIt = body.find("buffer");
  if (bufferIt == body.end()) {
    return createEventObject(body);
  }
  auto bufferHostObject = std::static_pointer_cast<AudioBufferHostObject>(
      std::get<std::shared_ptr<jsi::HostObject>>(bufferIt->second));
  return createEventObject(body, bufferHostObject->getSizeInBytes());
}

void AudioEventHandlerRegistry::dispatchHandler(
    AudioEvent eventName,
    const std::shared_ptr<jsi::Function> &handler,
    const std::unordered_map<std::string, EventValue> &body) {
  if (handler == nullptr || !handler->isFunction(*runtime_)) {
    return;
  }
  try {
    jsi::Object eventObject = makeEventObjectForDispatch(eventName, body);
    handler->call(*runtime_, eventObject);
  } catch (const std::exception &) {
    throw;
  } catch (...) {
    printf("Unknown exception occurred while invoking handler for event: %d\n", eventName);
  }
}

void AudioEventHandlerRegistry::invokeHandlerWithEventBody(
    AudioEvent eventName,
    const std::unordered_map<std::string, EventValue> &body) {
  if (runtime_ == nullptr) {
    return;
  }

  auto weakSelf = weak_from_this();

  callInvoker_->invokeAsync([weakSelf, eventName, body]() {
    auto self = weakSelf.lock();
    if (self == nullptr) {
      return;
    }
    auto it = self->eventHandlers_.find(eventName);
    if (it == self->eventHandlers_.end()) {
      return;
    }
    for (const auto &pair : it->second) {
      self->dispatchHandler(eventName, pair.second, body);
    }
  });
}

void AudioEventHandlerRegistry::invokeHandlerWithEventBody(
    AudioEvent eventName,
    uint64_t listenerId,
    const std::unordered_map<std::string, EventValue> &body) {
  if (runtime_ == nullptr) {
    return;
  }

  auto weakSelf = weak_from_this();

  callInvoker_->invokeAsync([weakSelf, eventName, listenerId, body]() {
    auto self = weakSelf.lock();
    if (self == nullptr) {
      return;
    }
    auto it = self->eventHandlers_.find(eventName);
    if (it == self->eventHandlers_.end()) {
      return;
    }
    auto handlerIt = it->second.find(listenerId);
    if (handlerIt == it->second.end()) {
      return;
    }
    self->dispatchHandler(eventName, handlerIt->second, body);
  });
}

jsi::Object AudioEventHandlerRegistry::createEventObject(
    const std::unordered_map<std::string, EventValue> &body) {
  auto eventObject = jsi::Object(*runtime_);

  for (const auto &pair : body) {
    const auto *name = pair.first.data();
    const auto &value = pair.second;

    if (std::holds_alternative<int>(value)) {
      eventObject.setProperty(*runtime_, name, std::get<int>(value));
    } else if (std::holds_alternative<double>(value)) {
      eventObject.setProperty(*runtime_, name, std::get<double>(value));
    } else if (std::holds_alternative<float>(value)) {
      eventObject.setProperty(*runtime_, name, std::get<float>(value));
    } else if (std::holds_alternative<bool>(value)) {
      eventObject.setProperty(*runtime_, name, std::get<bool>(value));
    } else if (std::holds_alternative<std::string>(value)) {
      eventObject.setProperty(*runtime_, name, std::get<std::string>(value));
    } else if (std::holds_alternative<std::shared_ptr<jsi::HostObject>>(value)) {
      auto hostObject = jsi::Object::createFromHostObject(
          *runtime_, std::get<std::shared_ptr<jsi::HostObject>>(value));
      eventObject.setProperty(*runtime_, name, hostObject);
    }
  }

  return eventObject;
}

jsi::Object AudioEventHandlerRegistry::createEventObject(
    const std::unordered_map<std::string, EventValue> &body,
    size_t memoryPressure) {
  auto eventObject = createEventObject(body);
  eventObject.setExternalMemoryPressure(*runtime_, memoryPressure);
  return eventObject;
}

} // namespace audioapi
