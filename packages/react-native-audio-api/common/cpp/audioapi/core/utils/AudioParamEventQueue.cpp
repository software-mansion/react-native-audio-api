#include <audioapi/core/utils/AudioParamEventQueue.h>
#include <algorithm>
#include <utility>

namespace audioapi {

AudioParamEventQueue::AudioParamEventQueue() : eventQueue_() {}

void AudioParamEventQueue::pushBack(ParamChangeEvent &&event) {
  if (eventQueue_.isEmpty()) {
    eventQueue_.pushBack(std::move(event));
    return;
  }
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
  eventQueue_.pushBack(std::move(event));
}

bool AudioParamEventQueue::popFront(ParamChangeEvent &event) {
  return eventQueue_.popFront(event);
}

void AudioParamEventQueue::cancelScheduledValues(double cancelTime) {
  while (!eventQueue_.isEmpty()) {
    auto &back = eventQueue_.peekBack();
    if (back.getEndTime() < cancelTime) {
      break;
    }
    if (back.getStartTime() >= cancelTime ||
        back.getType() == ParamChangeEventType::SET_VALUE_CURVE) {
      eventQueue_.popBack();
    }
  }
}

void AudioParamEventQueue::cancelAndHoldAtTime(double cancelTime, double &endTimeCache) {
  while (!eventQueue_.isEmpty()) {
    auto &back = eventQueue_.peekBack();
    if (back.getEndTime() < cancelTime || back.getStartTime() <= cancelTime) {
      break;
    }
    eventQueue_.popBack();
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

} // namespace audioapi
