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
    const auto *conflict = findEventInInterval(event.getStartTime(), event.getEndTime());
    if (conflict) {
      return Err(
          std::format(
              "Cannot schedule curve event from time {} to {} because it conflicts with an existing event of type {} at time {}.",
              event.getStartTime(),
              event.getEndTime(),
              toString(conflict->getType()),
              conflict->getAutomationEventTime()));
    }
  } else {
    const auto *conflict = findEventAtTime(event.getAutomationEventTime());
    if (conflict && conflict->getType() == AutomationEventType::SET_VALUE_CURVE) {
      return Err(
          std::format(
              "Cannot schedule event of type {} at time {} because it conflicts with an existing curve event from time {} to {}.",
              toString(event.getType()),
              event.getAutomationEventTime(),
              conflict->getStartTime(),
              conflict->getEndTime()));
    }
  }
  return Ok(None);
}

void AutomationControlQueue::cancelScheduledValues(double cancelTime) {
  while (!eventQueue_.isEmpty() && eventQueue_.peekBack().getAutomationEventTime() >= cancelTime) {
    eventQueue_.popBack();
  }
}

const AutomationEvent *AutomationControlQueue::findEventAtTime(double automationTime) const {
  for (const auto &event : eventQueue_) {
    if ((event.getType() == AutomationEventType::SET_VALUE_CURVE &&
         event.getStartTime() <= automationTime && automationTime <= event.getEndTime()) ||
        event.getAutomationEventTime() == automationTime) {
      return &event;
    }
  }
  return nullptr;
}

const AutomationEvent *AutomationControlQueue::findEventInInterval(double startTime, double endTime)
    const {
  for (const auto &event : eventQueue_) {
    if (event.getStartTime() >= startTime && event.getStartTime() < endTime) {
      return &event;
    }
  }
  return nullptr;
}

} // namespace audioapi
