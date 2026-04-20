#include <audioapi/core/types/ParamEventType.h>
#include <audioapi/core/utils/param/ParamControlQueue.h>
#include <audioapi/core/utils/param/ParamEvent.h>
#include <audioapi/utils/Result.hpp>
#include <cstddef>
#include <sstream>

namespace audioapi {

EventConflictResult ParamControlQueue::checkCurveExclusion(const ParamEvent &event) {
  if (event.getType() == ParamEventType::SET_VALUE_CURVE) {
    // For curve events, check for any event that occurs at or within the curve's time interval
    return isConflictInInterval(event, event.getStartTime(), event.getEndTime());
  }
  // For non-curve events check for curve events that conflict at the event's automationTime
  return isConflictAtTime(event, event.getAutomationTime());
}

void ParamControlQueue::purge(double currentTime) {
  eventQueue_.erase(eventQueue_.begin(), eventQueue_.lowerBound(currentTime));
}

EventConflictResult ParamControlQueue::isConflictAtTime(
    const ParamEvent &newEvent,
    double automationTime) {
  // Check if a SET_VALUE_CURVE that starts before automationTime extends into it
  auto it = eventQueue_.upperBound(automationTime);
  if (it != eventQueue_.begin()) {
    const auto &pred = *std::prev(it);
    if (pred.getType() == ParamEventType::SET_VALUE_CURVE && automationTime < pred.getEndTime()) {
      std::stringstream ss;
      ss << "Cannot schedule event of type " << toString(newEvent.getType()) << " at time "
         << newEvent.getAutomationTime()
         << " because it conflicts with an existing curve event from time " << pred.getStartTime()
         << " to " << pred.getEndTime() << ".";
      return Err(ss.str());
    }
  }
  return Ok(None);
}

EventConflictResult ParamControlQueue::isConflictInInterval(
    const ParamEvent &newEvent,
    double startTime,
    double endTime) {
  // Non-ramp events have automationTime == startTime, so lowerBound/lowerBound brackets them.
  // Ramp events have startTime == 0 (unresolved on the control thread), so getStartTime() > startTime
  // filters them out.
  for (auto it = eventQueue_.lowerBound(startTime), hi = eventQueue_.lowerBound(endTime); it != hi;
       ++it) {
    if (it->getStartTime() > startTime) {
      std::stringstream ss;
      ss << "Cannot schedule curve event from time " << newEvent.getStartTime() << " to "
         << newEvent.getEndTime() << " because it conflicts with an existing event of type "
         << toString(it->getType()) << " at time " << it->getAutomationTime() << ".";
      return Err(ss.str());
    }
  }
  return Ok(None);
}

} // namespace audioapi
