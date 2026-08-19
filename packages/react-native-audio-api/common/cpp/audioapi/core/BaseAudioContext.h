#pragma once

#include <audioapi/core/types/ContextPromiseTask.h>
#include <audioapi/core/types/ContextState.h>
#include <audioapi/core/types/OscillatorType.h>
#include <audioapi/core/utils/Constants.h>
#include <audioapi/core/utils/Disposer.hpp>
#include <audioapi/core/utils/graph/Graph.h>
#include <audioapi/events/AudioEvent.h>
#include <audioapi/events/DeferredEventQueue.hpp>
#include <audioapi/utils/AudioBuffer.hpp>
#include <audioapi/utils/CrossThreadEventScheduler.hpp>
#include <audioapi/utils/TaskOffloader.hpp>

#include <audioapi/utils/Macros.h>
#include <atomic>
#include <cassert>
#include <complex>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

namespace audioapi {

class IAudioEventHandlerRegistry;
class PeriodicWave;
class AudioDestinationNode;

class BaseAudioContext : public std::enable_shared_from_this<BaseAudioContext> {
 public:
  explicit BaseAudioContext(
      float sampleRate,
      const std::shared_ptr<IAudioEventHandlerRegistry> &audioEventHandlerRegistry);
  virtual ~BaseAudioContext() = default;
  DELETE_COPY_AND_MOVE(BaseAudioContext);

  ContextState getState() const;
  [[nodiscard]] bool isClosed() const {
    return state_.load(std::memory_order_acquire) == ContextState::CLOSED;
  }
  [[nodiscard]] float getSampleRate() const;
  [[nodiscard]] double getCurrentTime() const;
  [[nodiscard]] std::size_t getCurrentSampleFrame() const;

  void setState(ContextState state);

  [[nodiscard]] std::shared_ptr<PeriodicWave> createPeriodicWave(
      const std::vector<std::complex<float>> &complexData,
      bool disableNormalization,
      int length) const;

  std::shared_ptr<PeriodicWave> getBasicWaveForm(OscillatorType type);
  std::shared_ptr<utils::graph::Graph> getGraph() const;
  std::shared_ptr<IAudioEventHandlerRegistry> getAudioEventHandlerRegistry() const;
  utils::DisposerImpl<DISPOSER_PAYLOAD_SIZE> *getDisposer() const;

  /// @brief Assigns the audio destination node to the context.
  /// @param destination The audio destination node to be associated with the context.
  /// @note This method must be called before the audio context can be used for processing audio.
  virtual void initialize(const AudioDestinationNode *destination);

  void processAudioEvents() {
    // Drain JS-thread scheduler first, then the GC-thread scheduler.
    // This preserves causality for the common case where a JS-thread
    // produced event logically happens-before a GC-thread cleanup event
    // on the same node (e.g. `onEnded = id` followed by HostObject
    // finalization that clears the callback id).
    audioEventScheduler_.processAllEvents(*this);
    gcAudioEventScheduler_.processAllEvents(*this);
    deferredEvents_.dispatchDue(getCurrentTime());
  }

  /// @brief Queue for events whose emitter cannot fire them itself, dispatched
  /// as `currentTime` reaches their due time. Owned here because the render
  /// loop is the only thing that advances the clock.
  /// @note Render-serialized only, like the queue itself.
  [[nodiscard]] DeferredEventQueue &getDeferredEvents() {
    return deferredEvents_;
  }

  template <typename F>
  bool scheduleAudioEvent(F &&event) noexcept { // NOLINT(cppcoreguidelines-missing-std-forward)
    std::scoped_lock lock(driverMutex_);
    if (isDriverRunning()) {
      return audioEventScheduler_.scheduleEvent(std::forward<F>(event));
    }

    // Sole consumer while the driver is stopped: drain any control messages that
    // were queued before the driver stopped (or before this sync call), then
    // run the new event. Keeps FIFO with the SPSC even on the sync path.
    audioEventScheduler_.processAllEvents(*this);
    event(*this);
    return true;
  }

  /// @brief Schedule an audio event produced on the JS runtime's finalizer
  /// (GC) thread. Uses a dedicated SPSC channel so the main
  /// `audioEventScheduler_` stays single-producer (JS thread).
  ///
  /// Call this from JSI HostObject destructors (which on Hermes fire on the
  /// concurrent GC thread) instead of `scheduleAudioEvent`, otherwise the
  /// GC thread and the JS thread will race on the same SPSC channel.
  ///
  /// @note Unlike `scheduleAudioEvent`, this does NOT fall through to a
  /// synchronous `event(*this)` when the context is not RUNNING. The
  /// finalizer thread must never touch context state directly, and the
  /// event handler itself must be safe to skip if the audio thread never
  /// drains it (e.g. context already closed).
  template <typename F>
  bool scheduleGCEvent(F &&event) noexcept {
    return gcAudioEventScheduler_.scheduleEvent(std::forward<F>(event));
  }

  /// @brief Queue a lifecycle control message on the pending-promises worker thread.
  /// Serialized with other lifecycle ops via `driverMutex_`. Graph host-node cleanup
  /// (`collectDisposedNodes`) runs on the worker before the operation body.
  template <typename F>
  void scheduleContextPromise(F &&operation) {
    ContextPromiseTask task;
    task.operation = std::forward<F>(operation);
    pendingPromisesOffloader_->getSender()->send(std::move(task));
  }

  void processGraph(DSPAudioBuffer *buffer, int numFrames);

 protected:
  std::atomic<std::size_t> currentSampleFrame_{0};
  const AudioDestinationNode *destination_;

  /// Serializes context lifecycle and driver control across the JS thread,
  /// pending-promises worker, and offline render thread.
  mutable std::mutex driverMutex_;
  std::atomic<ContextState> state_;

  /// @brief Joins the pending-promises worker after draining any queued
  /// lifecycle tasks. Idempotent.
  ///
  /// Derived-class destructors MUST call this as their first teardown step:
  /// the drained task bodies lock `driverMutex_` and touch the player, graph,
  /// and disposer, all of which start being destroyed once the destructor bodies return.
  void joinPendingPromiseWorker() {
    pendingPromisesOffloader_->shutdown();
  }

  /// Debug-only: `driverMutex_` must already be held by the calling thread.
  void assertDriverMutexHeld() const {
#ifndef NDEBUG
    if (driverMutex_.try_lock()) {
      driverMutex_.unlock();
      assert(false && "driverMutex_ must be held by the current thread");
    }
#endif
  }

 private:
  std::atomic<float> sampleRate_;
  std::shared_ptr<IAudioEventHandlerRegistry> audioEventHandlerRegistry_;

  std::shared_ptr<PeriodicWave> cachedSineWave_ = nullptr;
  std::shared_ptr<PeriodicWave> cachedSquareWave_ = nullptr;
  std::shared_ptr<PeriodicWave> cachedSawtoothWave_ = nullptr;
  std::shared_ptr<PeriodicWave> cachedTriangleWave_ = nullptr;
  static constexpr size_t AUDIO_SCHEDULER_CAPACITY = 1024;
  static constexpr size_t GC_AUDIO_SCHEDULER_CAPACITY = 256;
  static constexpr size_t PENDING_PROMISES_CAPACITY = 64;
  static constexpr auto PENDING_PROMISES_OVERFLOW_STRATEGY =
      audioapi::channels::spsc::OverflowStrategy::WAIT_ON_FULL;
  static constexpr auto PENDING_PROMISES_WAIT_STRATEGY =
      audioapi::channels::spsc::WaitStrategy::ATOMIC_WAIT;

  std::unique_ptr<task_offloader::TaskOffloader<
      ContextPromiseTask,
      PENDING_PROMISES_OVERFLOW_STRATEGY,
      PENDING_PROMISES_WAIT_STRATEGY>>
      pendingPromisesOffloader_;

  CrossThreadEventScheduler<BaseAudioContext> audioEventScheduler_;
  /// Dedicated single-producer SPSC scheduler for events produced on the
  /// JS runtime's finalizer (GC) thread. Keeps `audioEventScheduler_`
  /// single-producer (JS thread) and avoids the MPSC-on-SPSC data race.
  CrossThreadEventScheduler<BaseAudioContext> gcAudioEventScheduler_;

  std::unique_ptr<utils::DisposerImpl<DISPOSER_PAYLOAD_SIZE>> disposer_;
  std::shared_ptr<utils::graph::Graph> graph_;

  DeferredEventQueue deferredEvents_;

  [[nodiscard]] virtual bool isDriverRunning() const = 0;
};

} // namespace audioapi
