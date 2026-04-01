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

  void cancelScheduledValues(double cancelTime) override;

  /// @brief Cancel scheduled parameter changes and hold the current value at the given time.
  /// @param cancelTime The time at which to cancel scheduled changes.
  void cancelAndHoldAtTime(double cancelTime, double &endTimeCache);

  bool push(RenderAutomationEvent &&event) override;

 private:
  inline void setEventEndValueToCurrentValue(RenderAutomationEvent &event);
};

} // namespace audioapi
