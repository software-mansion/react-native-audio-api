#include <audioapi/core/utils/automation/AutomationRenderEventFactory.hpp>
#include <audioapi/core/utils/automation/AutomationRenderQueue.h>
#include <cstddef>
#include <optional>
#include <utility>
#include "audioapi/core/types/AutomationEventType.h"

namespace audioapi {

std::optional<float> AutomationRenderQueue::computeValueAtTime(double time) {
  while (
      !eventQueue_.isEmpty() &&
      (!currentEvent_ ||
       (time >= currentEvent_->getEndTime() && eventQueue_.peekFront().getStartTime() <= time))) {
    RenderAutomationEvent next;
    eventQueue_.pop(next);
    currentEvent_ = std::move(next);
  }

  if (!currentEvent_) {
    return std::nullopt;
  }

  return currentEvent_->getCalculateValue()(
      currentEvent_->getStartTime(),
      currentEvent_->getEndTime(),
      currentEvent_->getStartValue(),
      currentEvent_->getEndValue(),
      time);
}

bool AutomationRenderQueue::push(RenderAutomationEvent &&event) {
  resolveEventValues(event);
  return eventQueue_.push(std::move(event));
}

void AutomationRenderQueue::resolveEventValues(RenderAutomationEvent &event) {
  auto it = eventQueue_.upper_bound(event.getAutomationTime());

  RenderAutomationEvent *predecessor = nullptr;
  if (it != eventQueue_.begin()) {
    predecessor = &eventQueue_.deref_mut(std::prev(it));
  } else if (currentEvent_) {
    predecessor = &currentEvent_.value();
  }

  if (predecessor != nullptr) {
    // Set startTime BEFORE startValue for ramps — startValue depends on the resolved startTime
    if (event.isRampType()) {
      event.setStartTime(predecessor->getEndTime());
    }

    event.setStartValue(getValueOfPreviousEventAt(*predecessor, event.getStartTime()));

    if (predecessor->getType() == AutomationEventType::SET_TARGET) {
      predecessor->setEndTime(event.getStartTime());
      predecessor->setEndValue(getValueOfPreviousEventAt(*predecessor, predecessor->getEndTime()));
    }
  } else {
    event.setStartValue(defaultValue_);
  }

  if (it != eventQueue_.end() && it->isRampType()) {
    auto *successor = &eventQueue_.deref_mut(it);
    successor->setStartTime(event.getEndTime());
    successor->setStartValue(event.getEndValue());
  }
}

float AutomationRenderQueue::getValueOfPreviousEventAt(RenderAutomationEvent &event, double time) {
  if (event.getType() == AutomationEventType::SET_TARGET) {
    return event.getCalculateValue()(
        event.getStartTime(), event.getEndTime(), event.getStartValue(), event.getEndValue(), time);
  }
  return event.getEndValue();
}

void AutomationRenderQueue::cancelScheduledValues(double cancelTime) {
  while (!eventQueue_.isEmpty() && eventQueue_.peekBack().getAutomationTime() >= cancelTime) {
    eventQueue_.popBack();
  }
}

void AutomationRenderQueue::cancelAndHoldAtTime(double cancelTime) {
  float holdValue = currentEvent_ ? currentEvent_->getStartValue() : 0.0f;

  // Check E2: first event with automationTime > cancelTime
  auto e2It = eventQueue_.upper_bound(cancelTime);

  if (e2It != eventQueue_.end() && e2It->isRampType()) {
    // E2 is a ramp — compute its value at cancelTime
    const auto &e2 = *e2It;
    holdValue = e2.getCalculateValue()(
        e2.getStartTime(), e2.getEndTime(), e2.getStartValue(), e2.getEndValue(), cancelTime);
  } else {
    // Hold value comes from E1 or currentEvent_
    auto e1It = (e2It != eventQueue_.begin()) ? std::prev(e2It) : eventQueue_.end();
    if (e1It != eventQueue_.end()) {
      holdValue = getValueOfPreviousEventAt(eventQueue_.deref_mut(e1It), cancelTime);
    } else if (currentEvent_) {
      holdValue = getValueOfPreviousEventAt(*currentEvent_, cancelTime);
    }
  }

  // Remove all events after cancelTime
  while (!eventQueue_.isEmpty() && eventQueue_.peekBack().getAutomationTime() > cancelTime) {
    eventQueue_.popBack();
  }

  // Insert hold event — resolveEventValues will set startValue from E1
  this->push(std::move(AutomationRenderEventFactory::createSetValueEvent(holdValue, cancelTime)));
}

} // namespace audioapi
