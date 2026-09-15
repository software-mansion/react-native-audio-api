#pragma once

#include <ReactCommon/CallInvoker.h>
#include <audioapi/events/AudioEvent.h>
#include <audioapi/events/AudioEventPayload.h>
#include <audioapi/events/AudioEventProducer.h>
#include <audioapi/events/IAudioEventHandlerRegistry.h>
#include <audioapi/libs/concurrentqueue/concurrentqueue.h>
#include <audioapi/libs/concurrentqueue/lightweightsemaphore.h>
#include <audioapi/utils/Macros.h>
#include <jsi/jsi.h>
#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>

namespace audioapi {
using namespace facebook;

/// @brief Registry for audio event handlers with built-in dispatcher.
///
/// Events flow from native code to JS listeners through a single lock-free queue:
///
///   [producers]  -->  dispatchQueue_  -->  workerThread_  -->  CallInvoker  -->  JS handlers
///
/// Both entry points enqueue a DispatchEvent and signal itemsAvailable_:
///
/// - dispatchEventFromAudioThread() — real-time audio thread (e.g. `processNode()`).
///   Enqueues through the caller's own AudioEventProducer so try_enqueue() is wait-free and
///   never allocates. Drops the event if the queue is full.
///
/// - dispatchEvent() — any other (non-RT) thread (worker, platform/JNI callbacks, recorder
///   cleanup, AudioAPIModule, etc.). Uses the implicit (multi-producer) enqueue path.
///
/// workerThread_ waits on itemsAvailable_, drains dispatchQueue_, and posts each event to the
/// JS thread via callInvoker_. Handlers are registered/unregistered on the JS thread, and
/// handleEventOnJSThread() looks up and invokes the matching jsi::Function.
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

  /// @brief Enqueue an event from a non-audio thread.
  bool dispatchEvent(
      AudioEvent eventName,
      uint64_t listenerId,
      AudioEventPayload &&payload) noexcept override;

  std::shared_ptr<AudioEventProducer> createAudioEventProducer() override;

  /// @brief Enqueue an event from the real-time audio thread.
  /// Wait-free and allocation-free on the calling thread; drops when the queue is full.
  bool dispatchEventFromAudioThread(
      AudioEventProducer &producer,
      AudioEvent eventName,
      uint64_t listenerId,
      AudioEventPayload &&payload) noexcept override;

 private:
  static constexpr size_t kDispatchCapacity = 8192;

  struct DispatchEvent {
    AudioEvent event{};
    uint64_t listenerId{kBroadcastListenerId};
    AudioEventPayload payload{EmptyPayload{}};
  };

  std::atomic<uint64_t> listenerIdCounter_{1};
  const std::shared_ptr<react::CallInvoker> callInvoker_;
  jsi::Runtime *runtime_;
  // Guards eventHandlers_. register/unregister run on the JS thread (and node
  // destruction may run on the AudioDestructor worker), while handleEventOnJSThread
  // reads it on the JS thread. The audio thread never touches the map (it only
  // enqueues into dispatchQueue_), so this mutex is never taken on the RT path.
  std::mutex eventHandlersMutex_;
  std::unordered_map<AudioEvent, std::unordered_map<uint64_t, std::shared_ptr<jsi::Function>>>
      eventHandlers_;

  // Single producer-to-consumer channel for every thread. Audio threads bind their own
  // AudioEventProducer to it; every other thread uses the implicit-producer path.
  moodycamel::ConcurrentQueue<DispatchEvent> dispatchQueue_;
  // Counts queued items; workerThread_ waits on it instead of busy-spinning.
  moodycamel::LightweightSemaphore itemsAvailable_;
  std::atomic<bool> isExiting_{false};
  // Drains dispatchQueue_ and posts each event to the JS thread via callInvoker_.
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
