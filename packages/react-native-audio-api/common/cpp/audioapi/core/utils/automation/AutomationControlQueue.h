#pragma once

#include <string>
#include "audioapi/core/utils/automation/AutomationEvent.h"
#include "audioapi/core/utils/automation/AutomationQueueBase.hpp"
#include "audioapi/utils/Result.hpp"

namespace audioapi {

using EventConflictResult = Result<NoneType, std::string>;

/// @brief A queue for managing audio parameter change events on the JS/control thread.
/// @note The invariant of the queue is that its internal buffer always contains non-overlapping events.
class AutomationControlQueue : public AutomationQueueBase<AutomationEvent> {
 public:
  /// @brief Validate if a new event can be added to the queue without violating curve exclusion rules.
  /// See: https://webaudio.github.io/web-audio-api/#automation-event-time
  /// @param event The new event to validate.
  /// @return Ok if the event can be added, Err with a message if it cannot be added.
  [[nodiscard]] EventConflictResult checkCurveExclusion(const AutomationEvent &event);

  /// @brief Remove all events with automationTime strictly before currentTime.
  /// @note Should be called before push to prevent the queue from filling up with past events.
  void purge(double currentTime);

 private:
  /// @brief Find an event that occurs at the given time. For curve events, checks if time is within the event's interval.
  /// @param time The time to check for an event.
  /// @return A pointer to the conflicting event, or nullptr if no conflict.
  [[nodiscard]] EventConflictResult isConflictAtTime(const AutomationEvent &event, double time);

  /// @brief Find an event that starts within the given time interval.
  /// @param startTime The start time of the interval.
  /// @param endTime The end time of the interval.
  /// @return A pointer to the conflicting event, or nullptr if no conflict.
  [[nodiscard]] EventConflictResult
  isConflictInInterval(const AutomationEvent &event, double startTime, double endTime);
};

} // namespace audioapi
