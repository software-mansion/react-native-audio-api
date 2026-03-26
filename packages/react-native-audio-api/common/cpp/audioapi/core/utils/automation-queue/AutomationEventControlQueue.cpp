#include "audioapi/core/utils/automation-queue/AutomationEventControlQueue.h"
#include <format>
#include <string>
#include "audioapi/core/types/AutomationEventType.h"
#include "audioapi/utils/Result.hpp"

namespace audioapi {

Result<NoneType, std::string> AutomationEventControlQueue::satisfiesCurveExclusion(
    const BaseAutomationEvent &event) {
  double newT = event.getStartTime();
  bool isSetValueCurveAtTime = (event.getType() == AutomationEventType::SET_VALUE_CURVE);
  double newD = isSetValueCurveAtTime ? event.getEndTime() - event.getStartTime() : 0.0;

  for (size_t i = 0; i < eventQueue_.size(); ++i) {
    const auto &existing = eventQueue_.peekAt(i);
    double existingT = existing.getStartTime();

    // 1. Check if existing curve blocks the new event
    // Any automation method called at a time in [T, T+D) of an existing curve is not allowed
    if (existing.getType() == AutomationEventType::SET_VALUE_CURVE) {
      double existingEndTime = existing.getEndTime();
      if (newT >= existingT && newT < existingEndTime) {
        return Err(
            std::format(
                "Cannot schedule event {} at time {} because it overlaps with an existing SetValueCurveAtTime event from {} to {}",
                static_cast<int>(event.getType()), // TODO add event type to string conversion
                newT,
                existingT,
                existingEndTime));
      }
    }

    // 2. If new event is a curve, existing events strictly inside (T, T+D) are not allowed
    if (isSetValueCurveAtTime) {
      if (existingT > newT && existingT < (newT + newD)) {
        return Err(
            std::format(
                "Cannot schedule SetValueCurveAtTime event from {} to {} because it overlaps with an existing event {} at time {}",
                newT,
                newT + newD,
                static_cast<int>(existing.getType()), // TODO add event type to string conversion
                existingT));
      }
    }
  }

  return Ok(None);
}

} // namespace audioapi
