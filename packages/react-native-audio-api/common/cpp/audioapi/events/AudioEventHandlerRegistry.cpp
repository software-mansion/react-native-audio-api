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
  uint64_t listenerId = listenerIdCounter_++;

  if (callInvoker_ == nullptr || runtime_ == nullptr) {
    // If callInvoker or runtime is not valid, we cannot register the handler
    return 0;
  }

  // Modify the eventHandlers_ map only on the main RN thread
  callInvoker_->invokeAsync([this, eventName, listenerId, handler]() {
    eventHandlers_[eventName][listenerId] = handler;
  });

  return listenerId;
}

void AudioEventHandlerRegistry::unregisterHandler(
    const std::string &eventName,
    uint64_t listenerId) {
  if (callInvoker_ == nullptr || runtime_ == nullptr) {
    // If callInvoker or runtime is not valid, we cannot unregister the handler
    return;
  }

  callInvoker_->invokeAsync([this, eventName, listenerId]() {
    auto it = eventHandlers_.find(eventName);

    if (it != eventHandlers_.end()) {
      return;
    }

    auto &handlersMap = it->second;
    auto handlerIt = handlersMap.find(listenerId);

    if (handlerIt != handlersMap.end()) {
      handlersMap.erase(handlerIt);
    }
  });
}

void AudioEventHandlerRegistry::invokeHandlerWithEventBody(
    const std::string &eventName,
    const std::unordered_map<std::string, EventValue> &body) {
  // callInvoker_ and runtime_ must be valid to invoke handlers
  // this might happen when react-native is reloaded or the app is closed
  if (callInvoker_ == nullptr || runtime_ == nullptr) {
    return;
  }

  // Do any logic regarding triggering the event on the main RN thread
  callInvoker_->invokeAsync([this, eventName, body]() {
    auto it = eventHandlers_.find(eventName);

    if (it == eventHandlers_.end()) {
      // If the event name is not registered, we can skip invoking handlers
      return;
    }

    auto handlersMap = it->second;

    for (const auto &pair : handlersMap) {
      auto handler = pair.second;

      if (!handler || !handler->isFunction(*runtime_)) {
        // If the handler is not valid, we can skip it
        continue;
      }

      auto eventObject = createEventObject(body);
      handler->call(*runtime_, eventObject);
    }
  });
}

void AudioEventHandlerRegistry::invokeHandlerWithEventBody(
    const std::string &eventName,
    uint64_t listenerId,
    const std::unordered_map<std::string, EventValue> &body) {
  // callInvoker_ and runtime_ must be valid to invoke handlers
  // this might happen when react-native is reloaded or the app is closed
  if (callInvoker_ == nullptr || runtime_ == nullptr) {
    return;
  }

  callInvoker_->invokeAsync([this, eventName, listenerId, body]() {
    auto it = eventHandlers_.find(eventName);

    if (it == eventHandlers_.end()) {
      // If the event name is not registered, we can skip invoking handlers
      return;
    }

    auto handlerIt = it->second.find(listenerId);

    if (handlerIt == it->second.end()) {
      // If the listener ID is not registered, we can skip invoking handlers
      return;
    }

    auto eventObject = createEventObject(body);
    handlerIt->second->call(*runtime_, eventObject);
  });
}

jsi::Object AudioEventHandlerRegistry::createEventObject(
    const std::unordered_map<std::string, EventValue> &body) {
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
    } else if (holds_alternative<std::shared_ptr<jsi::HostObject>>(value)) {
      auto hostObject = jsi::Object::createFromHostObject(
          *runtime_, get<std::shared_ptr<jsi::HostObject>>(value));
      eventObject.setProperty(*runtime_, name, hostObject);
    }
  }

  return eventObject;
}

} // namespace audioapi
