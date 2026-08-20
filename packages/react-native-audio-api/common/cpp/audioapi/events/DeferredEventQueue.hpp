#pragma once

#include <audioapi/events/AudioEvent.h>
#include <audioapi/events/AudioEventPayload.h>
#include <audioapi/events/IAudioEventHandlerRegistry.h>
#include <audioapi/utils/BoundedPriorityQueue.hpp>
#include <audioapi/utils/Macros.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>

namespace audioapi {

/// @brief Time-ordered queue of payload-less audio events waiting for the
/// context clock to reach their due time.
///
/// Exists for events whose emitter can no longer fire them itself: a source
/// node that finishes without ever rendering is disabled — and may be reaped
/// from the graph — long before its `onended` falls due, so the pending
/// dispatch has to outlive it. Entries therefore hold a plain callback id and
/// never a reference to the emitter; a dispatch whose handler has since been
/// unregistered is dropped by the registry on the JS thread.
///
/// @note Render-serialized only (audio thread, or the synchronous
/// `scheduleAudioEvent` path) — no lock, so no other thread may touch it.
class DeferredEventQueue {
 public:
  explicit DeferredEventQueue(std::shared_ptr<IAudioEventHandlerRegistry> registry)
      : registry_(std::move(registry)) {}

  /// @brief Queues @p event for dispatch once the clock reaches @p dueTime.
  /// @return False when there is nothing to queue (@p callbackId unset) or no
  /// room left for it — filling up takes a pathological burst of deferrals.
  /// @note Allocation-free, so a full queue drops rather than grows.
  bool defer(AudioEvent event, uint64_t callbackId, double dueTime) {
    if (callbackId == 0) {
      return false;
    }

    return pending_.push(
        DeferredEvent{.dueTime = dueTime, .event = event, .callbackId = callbackId});
  }

  /// @brief Fires every entry due at or before @p now, in due-time order.
  void dispatchDue(double now) {
    // Ordered by due time, so the first entry that is not due ends the sweep.
    while (!pending_.isEmpty() && pending_.peekFront().dueTime <= now) {
      DeferredEvent due{};
      pending_.pop(due);
      dispatch(due);
    }
  }

  [[nodiscard]] size_t pendingCount() const noexcept {
    return pending_.size();
  }

  /// @brief How many events may be pending before `defer` starts firing early.
  static constexpr size_t MAX_PENDING_EVENTS = 16;

 private:
  struct DeferredEvent {
    double dueTime;
    AudioEvent event;
    uint64_t callbackId;
  };

  struct ByDueTime {
    bool operator()(const DeferredEvent &a, const DeferredEvent &b) const {
      return a.dueTime < b.dueTime;
    }
  };

  void dispatch(const DeferredEvent &deferred) const {
    if (registry_ == nullptr) {
      return;
    }

    registry_->dispatchEventFromAudioThread(
        deferred.event, deferred.callbackId, AudioEventPayload{EmptyPayload{}});
  }

  BoundedPriorityQueue<DeferredEvent, MAX_PENDING_EVENTS, ByDueTime> pending_;
  std::shared_ptr<IAudioEventHandlerRegistry> registry_;
};

} // namespace audioapi
