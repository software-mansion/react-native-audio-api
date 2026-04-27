#pragma once

#include <audioapi/events/AudioEvent.h>
#include <audioapi/events/AudioEventPayload.h>
#include <audioapi/utils/Macros.h>
#include <jsi/jsi.h>
#include <cstdint>
#include <memory>

namespace audioapi {

class IAudioEventHandlerRegistry {
 public:
  IAudioEventHandlerRegistry() = default;
  virtual ~IAudioEventHandlerRegistry() = default;

  DELETE_COPY_AND_MOVE(IAudioEventHandlerRegistry);

  virtual uint64_t registerHandler(
      AudioEvent eventName,
      const std::shared_ptr<facebook::jsi::Function> &handler) = 0;

  virtual void unregisterHandler(AudioEvent eventName, uint64_t listenerId) = 0;

  /// @brief Enqueue an event for dispatch to JS handlers. Lock-free, allocation-free.
  /// @param listenerId Target handler. Pass kBroadcastListenerId (0) to invoke all handlers.
  /// @return true if enqueued; false if the internal channel is full (event dropped).
  virtual bool dispatchEvent(
      AudioEvent eventName,
      uint64_t listenerId,
      AudioEventPayload &&payload) noexcept = 0;
};

} // namespace audioapi
