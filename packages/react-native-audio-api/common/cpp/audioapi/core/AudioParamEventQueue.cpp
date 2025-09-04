#include <audioapi/core/AudioParamEventQueue.h>

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
    auto &front = eventQueue_.peekBack();
    if (front.getEndTime() < cancelTime) {
      break;
    }
    if (front.getStartTime() >= cancelTime ||
        front.getType() == ParamChangeEventType::SET_VALUE_CURVE) {
      eventQueue_.popBack();
    }
  }
}

void AudioParamEventQueue::cancelAndHoldAtTime(
    double cancelTime,
    double &endTimeCache) {
  while (!eventQueue_.isEmpty()) {
    auto &front = eventQueue_.peekBack();
    if (front.getEndTime() < cancelTime || front.getStartTime() <= cancelTime) {
      break;
    }
    eventQueue_.popBack();
  }

  if (eventQueue_.isEmpty()) {
    endTimeCache = cancelTime;
    return;
  }

  auto &back = eventQueue_.peekBackMut();
  back.setEndTime(std::min(cancelTime, back.getEndTime()));
}

} // namespace audioapi
