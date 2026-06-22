#include <audioapi/events/AudioEventHandlerRegistry.h>
#include <cstdio>
#include <memory>
#include <unordered_map>
#include <utility>

namespace audioapi {

AudioEventHandlerRegistry::AudioEventHandlerRegistry(
    jsi::Runtime *runtime,
    const std::shared_ptr<react::CallInvoker> &callInvoker)
    : callInvoker_(callInvoker),
      runtime_(runtime),
      dispatchQueue_(kDispatchCapacity),
      audioProducerToken_(dispatchQueue_) {
  // Dispatch worker: wait for an item, dequeue it, then hop to the JS thread.
  workerThread_ = std::thread([this]() {
    while (true) {
      itemsAvailable_.wait();
      if (isExiting_.load(std::memory_order_acquire)) {
        break;
      }

      DispatchEvent item;
      if (!dispatchQueue_.try_dequeue(item)) {
        continue;
      }

      auto weak = weak_from_this();
      callInvoker_->invokeAsync([weak, capturedItem = std::move(item)]() {
        if (auto self = weak.lock()) {
          self->handleEventOnJSThread(
              capturedItem.event, capturedItem.listenerId, capturedItem.payload);
        }
      });
    }
  });
}

AudioEventHandlerRegistry::~AudioEventHandlerRegistry() {
  isExiting_.store(true, std::memory_order_release);
  itemsAvailable_.signal(); // wake the worker so it observes isExiting_
  if (workerThread_.joinable()) {
    workerThread_.join();
  }
  eventHandlers_.clear();
}

uint64_t AudioEventHandlerRegistry::registerHandler(
    AudioEvent eventName,
    const std::shared_ptr<jsi::Function> &handler) {
  auto listenerId = listenerIdCounter_.fetch_add(1, std::memory_order_relaxed);

  if (runtime_ == nullptr) {
    return 0;
  }

  auto weakSelf = weak_from_this();
  callInvoker_->invokeAsync([weakSelf, eventName, listenerId, handler]() {
    if (auto self = weakSelf.lock()) {
      self->eventHandlers_[eventName][listenerId] = handler;
    }
  });

  return listenerId;
}

void AudioEventHandlerRegistry::unregisterHandler(AudioEvent eventName, uint64_t listenerId) {
  if (runtime_ == nullptr) {
    return;
  }

  auto weakSelf = weak_from_this();
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

bool AudioEventHandlerRegistry::dispatchEvent(
    AudioEvent eventName,
    uint64_t listenerId,
    AudioEventPayload &&payload) noexcept {
  if (runtime_ == nullptr) {
    return false;
  }
  if (!dispatchQueue_.try_enqueue(
          DispatchEvent{
              .event = eventName, .listenerId = listenerId, .payload = std::move(payload)})) {
    return false;
  }
  itemsAvailable_.signal();
  return true;
}

bool AudioEventHandlerRegistry::dispatchEventFromAudioThread(
    AudioEvent eventName,
    uint64_t listenerId,
    AudioEventPayload &&payload) noexcept {
  if (runtime_ == nullptr) {
    return false;
  }
  if (!dispatchQueue_.try_enqueue(
          audioProducerToken_,
          DispatchEvent{
              .event = eventName, .listenerId = listenerId, .payload = std::move(payload)})) {
    return false;
  }
  itemsAvailable_.signal();
  return true;
}

void AudioEventHandlerRegistry::handleEventOnJSThread(
    AudioEvent eventName,
    uint64_t listenerId,
    const AudioEventPayload &payload) {
  auto it = eventHandlers_.find(eventName);
  if (it == eventHandlers_.end()) {
    return;
  }

  if (listenerId == kBroadcastListenerId) {
    auto handlersCopy = it->second;
    for (const auto &pair : handlersCopy) {
      invokeHandler(pair.second, payload);
    }
  } else {
    auto handlerIt = it->second.find(listenerId);
    if (handlerIt != it->second.end()) {
      invokeHandler(handlerIt->second, payload);
    }
  }
}

void AudioEventHandlerRegistry::invokeHandler(
    const std::shared_ptr<jsi::Function> &handler,
    const AudioEventPayload &payload) {
  if (!handler || !handler->isFunction(*runtime_)) {
    return;
  }
  try {
    auto eventObject = buildJsiObject(payload);
    handler->call(*runtime_, eventObject);
  } catch (const std::exception &) {
    throw;
  } catch (...) {
    printf("Unknown exception occurred while invoking audio event handler\n");
  }
}

} // namespace audioapi
