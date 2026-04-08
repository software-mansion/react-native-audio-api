#include "audioapi/core/utils/automation/AutomationControlQueue.h"
#include <cstddef>
#include <sstream>
#include <string>
#include "audioapi/core/types/AutomationEventType.h"
#include "audioapi/core/utils/automation/AutomationEvent.hpp"
#include "audioapi/utils/Result.hpp"

namespace audioapi {

Result<NoneType, std::string> AutomationControlQueue::checkCurveExclusion(
    const AutomationEvent &event) {
  if (event.getType() == AutomationEventType::SET_VALUE_CURVE) {
    // For curve events, check for any event that occurs at or within the curve's time interval
    const auto *conflict = findEventInInterval(event.getStartTime(), event.getEndTime());
    if (conflict != nullptr) {
      std::stringstream ss;
      ss << "Cannot schedule curve event from time " << event.getStartTime() << " to "
         << event.getEndTime() << " because it conflicts with an existing event of type "
         << toString(conflict->getType()) << " at time " << conflict->getAutomationTime() << ".";
      return Err(ss.str());
    }
  } else {
    // For non-curve events check for curve events that conflict at the event's automationTime
    const auto *conflict = findEventAtTime(event.getAutomationTime());
    if ((conflict != nullptr) && conflict->getType() == AutomationEventType::SET_VALUE_CURVE) {
      std::stringstream ss;
      ss << "Cannot schedule event of type " << toString(event.getType()) << " at time "
         << event.getAutomationTime()
         << " because it conflicts with an existing curve event from time "
         << conflict->getStartTime() << " to " << conflict->getEndTime() << ".";
      return Err(ss.str());
    }
  }
  return Ok(None);
}

void AutomationControlQueue::purge(double currentTime) {
  eventQueue_.erase(eventQueue_.begin(), eventQueue_.lowerBound(currentTime));
}

const AutomationEvent *AutomationControlQueue::findEventAtTime(double automationTime) {
  // Check if a SET_VALUE_CURVE that starts before automationTime extends into it
  auto it = eventQueue_.upperBound(automationTime);
  if (it != eventQueue_.begin()) {
    const auto &pred = *std::prev(it);
    if (pred.getType() == AutomationEventType::SET_VALUE_CURVE &&
        automationTime <= pred.getEndTime()) {
      return &pred;
    }
  }

  // Check for an event with an exact automationTime match
  auto lo = eventQueue_.lowerBound(automationTime);
  if (lo != it) {
    return &(*lo);
  }

  return nullptr;
}

const AutomationEvent *AutomationControlQueue::findEventInInterval(
    double startTime,
    double endTime) {
  // Non-ramp events have automationTime == startTime, so lowerBound/lowerBound brackets them.
  // Ramp events have startTime == 0 (unresolved on the control thread), so getStartTime() >= startTime
  // filters them out.
  for (auto it = eventQueue_.lowerBound(startTime), hi = eventQueue_.lowerBound(endTime); it != hi;
       ++it) {
    if (it->getStartTime() >= startTime) {
      return &(*it);
    }
  }
  return nullptr;
}

} // namespace audioapi
