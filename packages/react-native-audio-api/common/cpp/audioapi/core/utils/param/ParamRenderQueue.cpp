#include <audioapi/core/types/ParamEventType.h>
#include <audioapi/core/utils/param/ParamQueueBase.hpp>
#include <audioapi/core/utils/param/ParamRenderEventFactory.hpp>
#include <audioapi/core/utils/param/ParamRenderQueue.h>
#include <cstddef>
#include <optional>
#include <utility>

namespace audioapi {

std::optional<float> ParamRenderQueue::computeValueAtTime(double time) {
  while (
      !eventQueue_.isEmpty() &&
      (!currentEvent_ ||
       (time >= currentEvent_->getEndTime() && eventQueue_.peekFront().getStartTime() <= time))) {
    RenderParamEvent next;
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

bool ParamRenderQueue::push(RenderParamEvent &&event) {
  resolveEventValues(event);
  return ParamQueueBase::push(std::move(event));
}

void ParamRenderQueue::resolveEventValues(RenderParamEvent &event) {
  auto it = eventQueue_.upperBound(event.getAutomationTime());

  if (it != eventQueue_.begin()) {
    // Case 1: there is a preceding event in the queue
    auto predIt = std::prev(it);

    // if the new event is a ramp resolve its startTime and startValue from the predecessor event
    if (event.isRampType()) {
      event.setStartTime(predIt->getEndTime());
    }
    event.setStartValue(getValueOfPreviousEventAt(*predIt, event.getStartTime()));

    // If the predecessor is a setTarget event, adjust its endTime and endValue to connect to the new event
    if (predIt->getType() == ParamEventType::SET_TARGET) {
      float newEndValue = getValueOfPreviousEventAt(*predIt, event.getStartTime());
      auto node = eventQueue_.extract(predIt);
      node.value().setEndTime(event.getStartTime());
      node.value().setEndValue(newEndValue);
      eventQueue_.insert(it, std::move(node));
    }
  } else if (currentEvent_) {
    // Case 2: no preceding event in queue, but currentEvent_ exists

    // if the new event is a ramp resolve its startTime and startValue from the predecessor event
    if (event.isRampType()) {
      event.setStartTime(currentEvent_->getEndTime());
    }
    event.setStartValue(getValueOfPreviousEventAt(*currentEvent_, event.getStartTime()));

    // If the predecessor is a setTarget event, adjust its endTime and endValue to connect to the new event
    if (currentEvent_->getType() == ParamEventType::SET_TARGET) {
      currentEvent_->setEndTime(event.getStartTime());
      currentEvent_->setEndValue(getValueOfPreviousEventAt(*currentEvent_, event.getStartTime()));
    }
  } else {
    // Case 3: no predecessor at all — fall back to default value
    event.setStartValue(defaultValue_);
  }

  // If the successor exists and is a ramp, reconnect its start to this event's end
  if (it != eventQueue_.end() && it->isRampType()) {
    auto hint = std::next(it);
    auto node = eventQueue_.extract(it);
    node.value().setStartTime(event.getEndTime());
    node.value().setStartValue(event.getEndValue());
    eventQueue_.insert(hint, std::move(node));
  }
}

float ParamRenderQueue::getValueOfPreviousEventAt(const RenderParamEvent &event, double time) {
  if (event.getType() == ParamEventType::SET_TARGET) {
    return event.getCalculateValue()(
        event.getStartTime(), event.getEndTime(), event.getStartValue(), event.getEndValue(), time);
  }
  return event.getEndValue();
}

void ParamRenderQueue::cancelAndHoldAtTime(double cancelTime) {
  // E2: first event with automationTime strictly after cancelTime
  auto e2It = eventQueue_.upperBound(cancelTime);

  if (e2It != eventQueue_.end() && e2It->isRampType()) {
    // Spec step 3: E2 is a ramp — truncate it to end at cancelTime
    float holdValue = e2It->getCalculateValue()(
        e2It->getStartTime(),
        e2It->getEndTime(),
        e2It->getStartValue(),
        e2It->getEndValue(),
        cancelTime);
    auto node = eventQueue_.extract(e2It);
    node.value().setEndTime(cancelTime);
    node.value().setEndValue(holdValue);
    auto insertPos = eventQueue_.upperBound(cancelTime);
    eventQueue_.insert(insertPos, std::move(node));
    // Step 5: remove everything strictly after cancelTime
    eventQueue_.erase(eventQueue_.upperBound(cancelTime), eventQueue_.end());
    return;
  }

  // Spec step 4: check E1 (last event with automationTime <= cancelTime)
  auto e1It = (e2It != eventQueue_.begin()) ? std::prev(e2It) : eventQueue_.end();

  if (e1It != eventQueue_.end()) {
    if (e1It->getType() == ParamEventType::SET_TARGET) {
      // Insert setValueAtTime to freeze the exponential approach
      float holdValue = getValueOfPreviousEventAt(*e1It, cancelTime);
      eventQueue_.erase(e2It, eventQueue_.end());
      this->push(ParamRenderEventFactory::createSetValueEvent(holdValue, cancelTime));
      return;
    }

    if (e1It->getType() == ParamEventType::SET_VALUE_CURVE && cancelTime <= e1It->getEndTime()) {
      // Truncate curve; compute holdValue using original endTime to preserve sampling behaviour
      float holdValue = e1It->getCalculateValue()(
          e1It->getStartTime(),
          e1It->getEndTime(),
          e1It->getStartValue(),
          e1It->getEndValue(),
          cancelTime);
      auto hint = std::next(e1It);
      auto node = eventQueue_.extract(e1It);
      node.value().setEndTime(cancelTime);
      node.value().setEndValue(holdValue);
      eventQueue_.insert(hint, std::move(node));
      // fall through to step 5
    }
    // All other E1 types (SET_VALUE, completed ramps): nothing to modify, fall through to step 5
  } else if (currentEvent_) {
    // No E1 in queue, but currentEvent_ exists — check if it needs to be truncated
    if (currentEvent_->getType() == ParamEventType::SET_TARGET) {
      float holdValue = getValueOfPreviousEventAt(*currentEvent_, cancelTime);
      eventQueue_.erase(eventQueue_.begin(), eventQueue_.end());
      this->push(ParamRenderEventFactory::createSetValueEvent(holdValue, cancelTime));
      return;
    }

    if (currentEvent_->getType() == ParamEventType::SET_VALUE_CURVE &&
        cancelTime <= currentEvent_->getEndTime()) {
      float holdValue = currentEvent_->getCalculateValue()(
          currentEvent_->getStartTime(),
          currentEvent_->getEndTime(),
          currentEvent_->getStartValue(),
          currentEvent_->getEndValue(),
          cancelTime);
      currentEvent_->setEndTime(cancelTime);
      currentEvent_->setEndValue(holdValue);
      // fall through to step 5
    }
  }

  // Step 5: remove all events strictly after cancelTime
  eventQueue_.erase(eventQueue_.upperBound(cancelTime), eventQueue_.end());
}

} // namespace audioapi
