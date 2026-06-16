#pragma once

#include <audioapi/events/AudioEvent.h>
#include <audioapi/events/AudioEventPayload.h>
#include <audioapi/utils/Macros.h>
#include <jsi/jsi.h>
#include <cstdint>
#include <memory>

// interface exists only for the sake of testing
// in every other case, AudioEventHandlerRegistry is used
// when looking for implementations, look for AudioEventHandlerRegistry

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

  virtual bool dispatchEvent(
      AudioEvent eventName,
      uint64_t listenerId,
      AudioEventPayload &&payload) noexcept = 0;

  virtual bool dispatchEventFromAudioThread(
      AudioEvent eventName,
      uint64_t listenerId,
      AudioEventPayload &&payload) noexcept = 0;
};

} // namespace audioapi
