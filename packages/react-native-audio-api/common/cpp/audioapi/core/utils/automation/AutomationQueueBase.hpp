#pragma once

#include <utility>
#include "audioapi/core/utils/Constants.h"
#include "audioapi/utils/BoundedPriorityQueue.hpp"

namespace audioapi {

template <typename TEvent>
concept AutomationEventConcept = requires(TEvent event) {
  { event.getAutomationTime() } -> std::convertible_to<double>;
};
template <AutomationEventConcept TEvent>
class AutomationQueueBase {
 public:
  AutomationQueueBase() = default;
  AutomationQueueBase(const AutomationQueueBase &) = delete;
  AutomationQueueBase &operator=(const AutomationQueueBase &) = delete;
  AutomationQueueBase(AutomationQueueBase &&) noexcept = delete;
  AutomationQueueBase &operator=(AutomationQueueBase &&) noexcept = delete;
  virtual ~AutomationQueueBase() = default;

  /// @brief Cancel scheduled parameter changes at or after the given time.
  /// @param cancelTime The time at which to cancel scheduled changes.
  virtual void cancelScheduledValues(double cancelTime) = 0;

  virtual bool push(TEvent &&event) {
    return eventQueue_.push(std::move(event));
  }

  virtual bool pop(TEvent &event) {
    return eventQueue_.pop(event);
  }

  /// @brief Check if the event queue is empty.
  [[nodiscard]] bool isEmpty() const noexcept {
    return eventQueue_.isEmpty();
  }

  /// @brief Check if the event queue is full.
  [[nodiscard]] bool isFull() const noexcept {
    return eventQueue_.isFull();
  }

  /// @brief Get the first event in the queue.
  /// @return The first event in the queue.
  [[nodiscard]] const TEvent &front() const noexcept {
    return eventQueue_.peekFront();
  }

  /// @brief Get the last event in the queue.
  /// @return The last event in the queue.
  [[nodiscard]] const TEvent &back() const noexcept {
    return eventQueue_.peekBack();
  }

 protected:
  struct EventComparator {
    using is_transparent = void;
    bool operator()(const TEvent &a, const TEvent &b) const {
      return a.getAutomationTime() < b.getAutomationTime();
    }
    bool operator()(const TEvent &a, double time) const {
      return a.getAutomationTime() < time;
    }
    bool operator()(double time, const TEvent &b) const {
      return time < b.getAutomationTime();
    }
  };

  BoundedPriorityQueue<TEvent, AUDIO_PARAM_MAX_QUEUED_EVENTS, EventComparator> eventQueue_;
};

} // namespace audioapi
