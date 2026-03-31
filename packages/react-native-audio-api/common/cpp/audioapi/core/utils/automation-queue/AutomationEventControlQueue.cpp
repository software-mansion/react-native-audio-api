#include "audioapi/core/utils/automation-queue/AutomationEventControlQueue.h"
#include <cstddef>
#include <format>
#include <string>
#include "audioapi/core/types/AutomationEventType.h"
#include "audioapi/core/utils/BaseAutomationEvent.hpp"
#include "audioapi/utils/Result.hpp"

namespace audioapi {

Result<NoneType, std::string> AutomationEventControlQueue::checkCurveExclusion(
    const BaseAutomationEvent &event) {
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

const BaseAutomationEvent *AutomationEventControlQueue::findEventAtTime(double time) const {
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

const BaseAutomationEvent *AutomationEventControlQueue::findEventInInterval(
    double startTime,
    double endTime) const {
  for (size_t i = 0; i < eventQueue_.size(); ++i) {
    const auto &event = eventQueue_.peekAt(i);
    if (event.getStartTime() >= startTime && event.getStartTime() < endTime) {
      return &event;
    }
  }
  return nullptr;
}

} // namespace audioapi
