#pragma once

#include <audioapi/events/AudioEvent.h>
#include <audioapi/events/AudioEventPayload.h>
#include <audioapi/events/AudioEventPayloadMapping.hpp>
#include <audioapi/events/IAudioEventHandlerRegistry.h>
#include <audioapi/utils/Macros.h>

#include <atomic>
#include <cstdint>
#include <memory>
#include <utility>

namespace audioapi {

template <AudioEvent Event>
class EventCaller {
 public:
  explicit EventCaller(const std::shared_ptr<IAudioEventHandlerRegistry> &audioEventHandlerRegistry)
      : eventHandlerRegistry_(audioEventHandlerRegistry) {}

  ~EventCaller() {
    unregisterCallback();
  }

  DELETE_COPY_AND_MOVE(EventCaller);

  void assignCallbackId(uint64_t callbackId) noexcept {
    if (getCallbackId() != callbackId) {
      unregisterCallback();
    }

    callbackId_.store(callbackId, std::memory_order_release);
  }

  [[nodiscard]] uint64_t getCallbackId() const noexcept {
    return callbackId_.load(std::memory_order_acquire);
  }

  [[nodiscard]] bool hasCallback() const noexcept {
    return getCallbackId() != 0;
  }

  void unregisterCallback() {
    auto id = getCallbackId();
    callbackId_.store(0, std::memory_order_release);
    if (eventHandlerRegistry_ == nullptr || id == 0) {
      return;
    }

    eventHandlerRegistry_->unregisterHandler(Event, id);
  }

  bool dispatchEmpty() const noexcept
    requires EventPayloadFor<Event, EmptyPayload>
  {
    return dispatch(EmptyPayload{});
  }

  bool dispatchEmptyFromAudioThread() const noexcept
    requires EventPayloadFor<Event, EmptyPayload>
  {
    return dispatchFromAudioThread(EmptyPayload{});
  }

  template <typename Payload>
    requires EventPayloadFor<Event, Payload>
  bool dispatch(Payload &&payload) const noexcept {
    const auto callbackId = getCallbackId();
    if (eventHandlerRegistry_ == nullptr || callbackId == 0) {
      return false;
    }

    return eventHandlerRegistry_->dispatchEvent(
        Event, callbackId, AudioEventPayload{std::forward<Payload>(payload)});
  }

  template <typename Payload>
    requires EventPayloadFor<Event, Payload>
  bool dispatchFromAudioThread(Payload &&payload) const noexcept {
    const auto callbackId = getCallbackId();
    if (eventHandlerRegistry_ == nullptr || callbackId == 0) {
      return false;
    }

    return eventHandlerRegistry_->dispatchEventFromAudioThread(
        Event, callbackId, AudioEventPayload{std::forward<Payload>(payload)});
  }

 private:
  std::shared_ptr<IAudioEventHandlerRegistry> eventHandlerRegistry_;
  std::atomic<uint64_t> callbackId_{0};
};

} // namespace audioapi
