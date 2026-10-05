#pragma once

#include <audioapi/events/AudioEvent.h>
#include <audioapi/events/AudioEventPayload.h>
#include <audioapi/utils/Macros.h>
#include <jsi/jsi.h>
#include <cstdint>
#include <memory>

// Production modules depend on this interface so tests
// can inject a mock. The concrete AudioEventHandlerRegistry is
// constructed only at platform bootstrap (AudioAPIModule / WPT install).

namespace audioapi {

class AudioEventProducer;

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

  /// @brief Creates a dispatch lane for one audio thread. Every owner of an audio
  /// thread needs its own — see AudioEventProducer for why sharing one corrupts the queue.
  virtual std::shared_ptr<AudioEventProducer> createAudioEventProducer() = 0;

  /// @param producer The calling audio thread's own producer, never one shared with
  /// another thread.
  virtual bool dispatchEventFromAudioThread(
      AudioEventProducer &producer,
      AudioEvent eventName,
      uint64_t listenerId,
      AudioEventPayload &&payload) noexcept = 0;
};

} // namespace audioapi
