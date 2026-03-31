#include "audioapi/core/utils/automation/AutomationControlQueue.h"
#include <cstddef>
#include <format>
#include <string>
#include <utility>
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

void AutomationControlQueue::cancelAutomationEvents(double cancelTime) {
  // BoundedPriorityQueue has no erase; drain into a temp buffer, keep survivors, rebuild.
  AutomationEvent survivors[32];
  size_t count = 0;

  AutomationEvent event;
  while (eventQueue_.pop(event)) {
    if (event.getAutomationEventTime() < cancelTime) {
      survivors[count++] = std::move(event);
    }
  }

  for (size_t i = 0; i < count; ++i) {
    eventQueue_.push(std::move(survivors[i]));
  }
}

const AutomationEvent *AutomationControlQueue::findEventAtTime(double time) const {
  for (size_t i = 0; i < eventQueue_.size(); ++i) {
    const auto &event = eventQueue_.peekAt(i);
    if ((event.getType() == AutomationEventType::SET_VALUE_CURVE && event.getStartTime() <= time &&
         time <= event.getEndTime()) ||
        event.getStartTime() == time) {
      return &event;
    }
  }
  return nullptr;
}

const AutomationEvent *AutomationControlQueue::findEventInInterval(double startTime, double endTime)
    const {
  for (size_t i = 0; i < eventQueue_.size(); ++i) {
    const auto &event = eventQueue_.peekAt(i);
    if (event.getStartTime() >= startTime && event.getStartTime() < endTime) {
      return &event;
    }
  }
  return nullptr;
}

} // namespace audioapi
