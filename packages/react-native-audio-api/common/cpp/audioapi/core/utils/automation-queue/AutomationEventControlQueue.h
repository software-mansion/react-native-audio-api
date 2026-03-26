#pragma once

#include <audioapi/core/types/AutomationEventType.h>
#include <audioapi/core/utils/BaseAutomationEvent.hpp>
#include <audioapi/core/utils/automation-queue/AutomationBaseQueue.hpp>
#include <audioapi/utils/Result.hpp>
#include <string>

namespace audioapi {

/// @brief A queue for managing audio parameter change events on the JS/control thread.
/// @note The invariant of the queue is that its internal buffer always contains non-overlapping events.
class AutomationEventControlQueue : public AutomationBaseQueue<BaseAutomationEvent> {
 public:
  explicit AutomationEventControlQueue() = default;

  /// @brief Validate if a new event can be added to the queue without violating curve exclusion rules.
  /// @param event The new event to validate.
  /// @return Ok if the event can be added, Err with a message if it cannot be added.
  [[nodiscard]] Result<NoneType, std::string> satisfiesCurveExclusion(
      const BaseAutomationEvent &event);

  /// @brief Cancel scheduled parameter changes at or after the given time.
  /// @param cancelTime The time at which to cancel scheduled changes.
  void cancelAutomationEvents(double cancelTime);

 private:
};

} // namespace audioapi
