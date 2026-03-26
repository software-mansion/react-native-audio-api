#pragma once

#include "audioapi/utils/BoundedPriorityQueue.hpp"

namespace audioapi {

template <typename TEvent>
class AutomationBaseQueue {
 protected:
  struct EventComparator {
    bool operator()(const TEvent &a, const TEvent &b) const {
      return a.getAutomationEventTime() < b.getAutomationEventTime();
    }
  };

  BoundedPriorityQueue<TEvent, 32, EventComparator> eventQueue_;

 public:
  /// @brief Gets the end time for a given event as if it were to be added to the queue.
  /// @param event The event for which to calculate the end time.
  /// @return The calculated end time for the event.
  [[nodiscard]] double getEndTimeForEvent(const TEvent &event) {
    /// TODO: Implement logic to calculate end time for different event types based on the event properties and the current state of the queue.
    return 0.0;
  };

  /// @brief Gets the start time for a given event as if it were to be added to the queue.
  /// @param event The event for which to calculate the start time.
  /// @return The calculated start time for the event.
  [[nodiscard]] double getStartTimeForEvent(const TEvent &event) {
    /// TODO: Implement logic to calculate start time for different event types based on the event properties and the current state of the queue.
    return 0.0;
  };
};

} // namespace audioapi
