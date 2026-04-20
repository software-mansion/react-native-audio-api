#pragma once

#include <audioapi/core/utils/param/ParamEvent.h>
#include <audioapi/core/utils/param/ParamQueueBase.hpp>
#include <audioapi/utils/Result.hpp>
#include <string>

namespace audioapi {

using EventConflictResult = Result<NoneType, std::string>;

/// @brief A queue for managing audio parameter change events on the JS/control thread.
/// @note The invariant of the queue is that its internal buffer always contains non-overlapping events.
class ParamControlQueue : public ParamQueueBase<ParamEvent> {
 public:
  /// @brief Validate if a new event can be added to the queue without violating curve exclusion rules.
  /// See: https://webaudio.github.io/web-audio-api/#automation-event-time
  /// @param event The new event to validate.
  /// @return Ok if the event can be added, Err with a message if it cannot be added.
  [[nodiscard]] EventConflictResult checkCurveExclusion(const ParamEvent &event);

  /// @brief Remove all events with automationTime strictly before currentTime.
  /// @note Should be called before push to prevent the queue from filling up with past events.
  void purge(double currentTime);

 private:
  /// @brief Check if a non-curve event at the given time conflicts with an existing curve event.
  /// @param event The new event being scheduled.
  /// @param time The automationTime of the new event.
  [[nodiscard]] EventConflictResult isConflictAtTime(const ParamEvent &event, double time);

  /// @brief Check if a curve event over [startTime, endTime) conflicts with any existing event.
  /// @param event The new curve event being scheduled.
  /// @param startTime Start of the curve interval.
  /// @param endTime End of the curve interval.
  [[nodiscard]] EventConflictResult
  isConflictInInterval(const ParamEvent &event, double startTime, double endTime);
};

} // namespace audioapi
