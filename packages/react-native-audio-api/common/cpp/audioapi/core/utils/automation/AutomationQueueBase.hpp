#pragma once

#include <utility>
#include "audioapi/utils/BoundedPriorityQueue.hpp"

namespace audioapi {

template <typename TEvent>
class AutomationQueueBase {
 public:
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
  [[nodiscard]] inline bool isEmpty() const noexcept {
    return eventQueue_.isEmpty();
  }

  /// @brief Check if the event queue is full.
  [[nodiscard]] inline bool isFull() const noexcept {
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
    bool operator()(const TEvent &a, const TEvent &b) const {
      return a.getAutomationEventTime() < b.getAutomationEventTime();
    }
  };

  BoundedPriorityQueue<TEvent, 32, EventComparator> eventQueue_;
};

} // namespace audioapi
