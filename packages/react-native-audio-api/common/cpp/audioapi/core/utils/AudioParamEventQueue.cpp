#include <audioapi/core/utils/AudioParamEventQueue.h>
#include <algorithm>
#include <format>
#include <string>
#include <utility>
#include "audioapi/utils/Result.hpp"

namespace audioapi {

AudioParamEventQueue::AudioParamEventQueue() : eventQueue_() {}

Result<NoneType, std::string> AudioParamEventQueue::push(ParamChangeEvent &&event) {
  if (eventQueue_.isEmpty()) {
    eventQueue_.push(std::move(event));
    return Ok(None);
  }

  auto isValid = satisfiesCurveExclusion(event);
  if (isValid.is_err()) {
    return Err(isValid.unwrap_err());
  }

  setEventEndValueToCurrentValue(event);
  eventQueue_.push(std::move(event));
  return Ok(None);
}

bool AudioParamEventQueue::pop(ParamChangeEvent &event) {
  return eventQueue_.pop(event);
}

void AudioParamEventQueue::cancelScheduledValues(double cancelTime) {
  while (!eventQueue_.isEmpty()) {
    auto &back = eventQueue_.peekBack();
    if (back.getEndTime() < cancelTime) {
      break;
    }
    if (back.getStartTime() >= cancelTime ||
        back.getType() == ParamChangeEventType::SET_VALUE_CURVE) {
      eventQueue_.pop();
    }
  }
}

void AudioParamEventQueue::cancelAndHoldAtTime(double cancelTime, double &endTimeCache) {
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

void AudioParamEventQueue::setEventEndValueToCurrentValue(ParamChangeEvent &event) {
  auto &prev = eventQueue_.peekBackMut();
  if (prev.getType() == ParamChangeEventType::SET_TARGET) {
    prev.setEndTime(event.getStartTime());
    // Calculate what the SET_TARGET value would be at the new event's start
    // time
    prev.setEndValue(prev.getCalculateValue()(
        prev.getStartTime(),
        prev.getEndTime(),
        prev.getStartValue(),
        prev.getEndValue(),
        event.getStartTime()));
  }
  event.setStartValue(prev.getEndValue());
}

Result<NoneType, std::string> AudioParamEventQueue::satisfiesCurveExclusion(
    const ParamChangeEvent &event) const {
  double newT = event.getStartTime();
  bool isSetValueCurveAtTime = (event.getType() == ParamChangeEventType::SET_VALUE_CURVE);
  double newD = isSetValueCurveAtTime ? event.getEndTime() - event.getStartTime() : 0.0;

  for (size_t i = 0; i < eventQueue_.size(); ++i) {
    const auto &existing = eventQueue_.peekAt(i);
    double existingT = existing.getStartTime();

    // 1. Check if existing curve blocks the new event
    // Any automation method called at a time in [T, T+D) of an existing curve is not allowed
    if (existing.getType() == ParamChangeEventType::SET_VALUE_CURVE) {
      double existingD = existing.getEndTime() -
          existing.getStartTime(); // TODO: is arthimetic on floating point safe here?
      if (newT >= existingT && newT < (existingT + existingD)) {
        return Err(
            std::format(
                "Cannot schedule event {} at time {} because it overlaps with an existing SetValueCurveAtTime event from {} to {}",
                static_cast<int>(event.getType()), // TODO add event type to string conversion
                newT,
                existingT,
                existingT + existingD));
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