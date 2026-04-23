#include <audioapi/core/types/ParamEventType.h>
#include <audioapi/core/utils/param/ParamControlQueue.h>
#include <audioapi/core/utils/param/ParamEvent.h>
#include <audioapi/utils/Result.hpp>
#include <cstddef>
#include <sstream>

namespace audioapi {

EventConflictResult ParamControlQueue::checkCurveExclusion(const ParamEvent &event) {
  if (event.getType() == ParamEventType::SET_VALUE_CURVE) {
    // Check if a new curve would start at a time that conflicts with an existing curve event
    auto startConflict = isConflictAtTime(event, event.getStartTime());
    if (startConflict.is_err()) {
      return startConflict;
    }
    // Curve rule: events strictly in (T, T+D) are conflicts; events exactly at T are allowed.
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
  // Per spec [T, T+D): events AT T are conflicts, so use upperBound to include the predecessor at T.
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
  auto it = eventQueue_.upperBound(startTime);
  // If there is an event in (T, T+D) then the new curve event is invalid
  while (it != eventQueue_.end() && it->getAutomationTime() < endTime) {
    if (it->getAutomationTime() > startTime) {
      std::stringstream ss;
      ss << "Cannot schedule curve event from time " << newEvent.getStartTime() << " to "
         << newEvent.getEndTime() << " because it conflicts with an existing event of type "
         << toString(it->getType()) << " at time " << it->getAutomationTime() << ".";
      return Err(ss.str());
    }
    ++it;
  }
  return Ok(None);
}

} // namespace audioapi
