#include <audioapi/core/utils/automation-queue/AutomationEventRenderQueue.h>
#include <algorithm>
#include <utility>
#include "audioapi/core/types/AutomationEventType.h"

namespace audioapi {

void AutomationEventRenderQueue::push(RenderAutomationEvent &&event) {
  if (eventQueue_.isEmpty()) {
    eventQueue_.push(std::move(event));
  }
  setEventEndValueToCurrentValue(event);
  eventQueue_.push(std::move(event));
}

bool AutomationEventRenderQueue::pop(RenderAutomationEvent &event) {
  return eventQueue_.pop(event);
}

void AutomationEventRenderQueue::cancelScheduledValues(double cancelTime) {
  while (!eventQueue_.isEmpty()) {
    auto &back = eventQueue_.peekBack();
    if (back.getEndTime() < cancelTime) {
      break;
    }
    if (back.getStartTime() >= cancelTime ||
        back.getType() == AutomationEventType::SET_VALUE_CURVE) {
      eventQueue_.pop();
    }
  }
}

void AutomationEventRenderQueue::cancelAndHoldAtTime(double cancelTime, double &endTimeCache) {
  while (!eventQueue_.isEmpty()) {
    auto &back = eventQueue_.peekBack();
    if (back.getEndTime() < cancelTime || back.getStartTime() <= cancelTime) {
      break;
    }
    eventQueue_.pop();
  }

  if (eventQueue_.isEmpty()) {
    endTimeCache = cancelTime;
    return;
  }

  auto &back = eventQueue_.peekBackMut();
  back.setEndValue(back.getCalculateValue()(
      back.getStartTime(),
      back.getEndTime(),
      back.getStartValue(),
      back.getEndValue(),
      cancelTime));
  back.setEndTime(std::min(cancelTime, back.getEndTime()));
}

void AutomationEventRenderQueue::setEventEndValueToCurrentValue(RenderAutomationEvent &event) {
  auto &prev = eventQueue_.peekBackMut();
  if (prev.getType() == AutomationEventType::SET_TARGET) {
    prev.setEndTime(event.getStartTime());
    // Calculate what the SET_TARGET value would be at the new event's start time
    prev.setEndValue(prev.getCalculateValue()(
        prev.getStartTime(),
        prev.getEndTime(),
        prev.getStartValue(),
        prev.getEndValue(),
        event.getStartTime()));
  }
  event.setStartValue(prev.getEndValue());
}

} // namespace audioapi
