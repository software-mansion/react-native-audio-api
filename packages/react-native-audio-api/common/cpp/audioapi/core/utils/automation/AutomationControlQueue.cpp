#include "audioapi/core/utils/automation/AutomationControlQueue.h"
#include <cstddef>
#include <format>
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
      return Err(
          std::format(
              "Cannot schedule curve event from time {} to {} because it conflicts with an existing event of type {} at time {}.",
              event.getStartTime(),
              event.getEndTime(),
              toString(conflict->getType()),
              conflict->getAutomationTime()));
    }
  } else {
    // For non-curve events check for curve events that conflict at the event's automationTime
    const auto *conflict = findEventAtTime(event.getAutomationTime());
    if ((conflict != nullptr) && conflict->getType() == AutomationEventType::SET_VALUE_CURVE) {
      return Err(
          std::format(
              "Cannot schedule event of type {} at time {} because it conflicts with an existing curve event from time {} to {}.",
              toString(event.getType()),
              event.getAutomationTime(),
              conflict->getStartTime(),
              conflict->getEndTime()));
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
