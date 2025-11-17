#include <audioapi/HostObjects/sources/AudioBufferHostObject.h>
#include <audioapi/events/AudioEventHandlerRegistry.h>
#include <memory>
#include <string>
#include <unordered_map>

namespace audioapi {

AudioEventHandlerRegistry::AudioEventHandlerRegistry(
    jsi::Runtime *runtime,
    const std::shared_ptr<react::CallInvoker> &callInvoker)
    : IAudioEventHandlerRegistry() {
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

    if (it == eventHandlers_.end()) {
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

      try {
        jsi::Object eventObject(*runtime_);
        // handle special logic for microphone, because we pass audio buffer
        // which has significant size
        if (eventName.compare("audioReady") == 0) {
          auto bufferIt = body.find("buffer");
          if (bufferIt != body.end()) {
            auto bufferHostObject = std::static_pointer_cast<AudioBufferHostObject>(
                std::get<std::shared_ptr<jsi::HostObject>>(bufferIt->second));
            eventObject = createEventObject(body, bufferHostObject->getSizeInBytes());
          }
        } else {
          eventObject = createEventObject(body);
        }
        handler->call(*runtime_, eventObject);
      } catch (const std::exception &e) {
        // re-throw the exception to be handled by the caller
        // std::exception is safe to parse by the rn bridge
        throw;
      } catch (...) {
        printf(
            "Unknown exception occurred while invoking handler for event: %s\n", eventName.c_str());
      }
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

    // Depending on how the AudioBufferSourceNode is handled on the JS side,
    // it sometimes might enter race condition where the ABSN is deleted on JS
    // side, but it is still processed on the audio thread, leading to a crash
    // when f.e. `positionChanged` event is triggered.

    // In case of debugging this, please increment the hours spent counter

    // Hours spent on this: 8
    try {
      if (!handlerIt->second || !handlerIt->second->isFunction(*runtime_)) {
        // If the handler is not valid, we can skip it
        return;
      }
      jsi::Object eventObject(*runtime_);
      // handle special logic for microphone, because we pass audio buffer which
      // has significant size
      if (eventName.compare("audioReady") == 0) {
        auto bufferIt = body.find("buffer");
        if (bufferIt != body.end()) {
          auto bufferHostObject = std::static_pointer_cast<AudioBufferHostObject>(
              std::get<std::shared_ptr<jsi::HostObject>>(bufferIt->second));
          eventObject = createEventObject(body, bufferHostObject->getSizeInBytes());
        }
      } else {
        eventObject = createEventObject(body);
      }
      handlerIt->second->call(*runtime_, eventObject);
    } catch (const std::exception &e) {
      // re-throw the exception to be handled by the caller
      // std::exception is safe to parse by the rn bridge
      throw;
    } catch (...) {
      printf(
          "Unknown exception occurred while invoking handler for event: %s\n", eventName.c_str());
    }
  });
}

jsi::Object AudioEventHandlerRegistry::createEventObject(
    const std::unordered_map<std::string, EventValue> &body) {
  auto eventObject = jsi::Object(*runtime_);

  for (const auto &pair : body) {
    const auto name = pair.first.data();
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
