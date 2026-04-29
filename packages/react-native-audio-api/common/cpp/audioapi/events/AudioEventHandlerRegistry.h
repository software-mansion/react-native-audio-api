#pragma once

#include <ReactCommon/CallInvoker.h>
#include <audioapi/events/AudioEvent.h>
#include <audioapi/events/AudioEventPayload.h>
#include <audioapi/events/IAudioEventHandlerRegistry.h>
#include <audioapi/utils/Macros.h>
#include <audioapi/utils/SpscChannel.hpp>
#include <jsi/jsi.h>
#include <atomic>
#include <memory>
#include <thread>
#include <unordered_map>

inline constexpr auto EVENT_DISPATCHER_SPSC_OVERFLOW_STRATEGY =
    audioapi::channels::spsc::OverflowStrategy::WAIT_ON_FULL;
inline constexpr auto EVENT_DISPATCHER_SPSC_WAIT_STRATEGY =
    audioapi::channels::spsc::WaitStrategy::ATOMIC_WAIT;

namespace audioapi {
using namespace facebook;

/// @brief Registry for audio event handlers with built-in dispatcher.
///
/// Handlers are registered/unregistered on the JS thread.
/// dispatchEvent() is lock-free and allocation-free — can be called from the
/// Audio Thread.
class AudioEventHandlerRegistry : public IAudioEventHandlerRegistry,
                                  public std::enable_shared_from_this<AudioEventHandlerRegistry> {
 public:
  explicit AudioEventHandlerRegistry(
      jsi::Runtime *runtime,
      const std::shared_ptr<react::CallInvoker> &callInvoker);
  ~AudioEventHandlerRegistry() override;

  DELETE_COPY_AND_MOVE(AudioEventHandlerRegistry);

  uint64_t registerHandler(AudioEvent eventName, const std::shared_ptr<jsi::Function> &handler)
      override;

  void unregisterHandler(AudioEvent eventName, uint64_t listenerId) override;

  bool dispatchEvent(
      AudioEvent eventName,
      uint64_t listenerId,
      AudioEventPayload &&payload) noexcept override;

 private:
  static constexpr size_t kDispatchCapacity = 256;

  struct DispatchEvent {
    AudioEvent event{};
    uint64_t listenerId{kBroadcastListenerId};
    AudioEventPayload payload{EmptyPayload{}};
  };

  using Sender = channels::spsc::Sender<
      DispatchEvent,
      EVENT_DISPATCHER_SPSC_OVERFLOW_STRATEGY,
      EVENT_DISPATCHER_SPSC_WAIT_STRATEGY>;
  using Receiver = channels::spsc::Receiver<
      DispatchEvent,
      EVENT_DISPATCHER_SPSC_OVERFLOW_STRATEGY,
      EVENT_DISPATCHER_SPSC_WAIT_STRATEGY>;

  std::atomic<uint64_t> listenerIdCounter_{1};
  const std::shared_ptr<react::CallInvoker> callInvoker_;
  jsi::Runtime *runtime_;
  std::unordered_map<AudioEvent, std::unordered_map<uint64_t, std::shared_ptr<jsi::Function>>>
      eventHandlers_;

  Sender sender_;
  Receiver receiver_;
  std::atomic<bool> isExiting_;
  std::thread workerThread_;

  void handleEventOnJSThread(
      AudioEvent eventName,
      uint64_t listenerId,
      const AudioEventPayload &payload);
  void invokeHandler(
      const std::shared_ptr<jsi::Function> &handler,
      const AudioEventPayload &payload);

  jsi::Object buildJsiObject(const AudioEventPayload &payload) {
    return std::visit(
        [this](auto &&p) -> jsi::Object { return p.toJsiObject(*runtime_); }, payload);
  }
};

} // namespace audioapi
