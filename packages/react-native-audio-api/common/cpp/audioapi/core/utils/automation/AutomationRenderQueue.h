#pragma once

#include <audioapi/core/types/AutomationEventType.h>
#include <audioapi/core/utils/automation/AutomationQueueBase.hpp>
#include <audioapi/core/utils/automation/RenderAutomationEvent.hpp>

namespace audioapi {

/// @brief A queue for managing audio parameter change events on the audio render thread.
/// @note The invariant of the queue is that its internal buffer always contains non-overlapping events.
class AutomationRenderQueue : public AutomationQueueBase<RenderAutomationEvent> {
 public:
  explicit AutomationRenderQueue() = default;

  /// @brief Push a new event to the back of the queue.
  /// @note Handles connecting the start value of the new event to the end value of the last event in the queue.
  void push(RenderAutomationEvent &&event);

  /// @brief Pop the front event from the queue.
  /// @return True if the pop was successful, false if the queue was empty.
  bool pop(RenderAutomationEvent &event);

  /// @brief Cancel scheduled parameter changes at or after the given time.
  /// @param cancelTime The time at which to cancel scheduled changes.
  void cancelScheduledValues(double cancelTime);

  /// @brief Cancel scheduled parameter changes and hold the current value at the given time.
  /// @param cancelTime The time at which to cancel scheduled changes.
  void cancelAndHoldAtTime(double cancelTime, double &endTimeCache);

  /// @brief Get the first event in the queue.
  /// @return The first event in the queue.
  inline const RenderAutomationEvent &front() const noexcept {
    return eventQueue_.peekFront();
  }

  /// @brief Get the last event in the queue.
  /// @return The last event in the queue.
  inline const RenderAutomationEvent &back() const noexcept {
    return eventQueue_.peekBack();
  }

  /// @brief Check if the event queue is empty.
  /// @return True if the queue is empty, false otherwise.
  inline bool isEmpty() const noexcept {
    return eventQueue_.isEmpty();
  }

  /// @brief Check if the event queue is full.
  /// @return True if the queue is full, false otherwise.
  inline bool isFull() const noexcept {
    return eventQueue_.isFull();
  }

 private:
  inline void setEventEndValueToCurrentValue(RenderAutomationEvent &event);
};

} // namespace audioapi
