#include <audioapi/core/AudioParamEventQueue.h>

namespace audioapi {

AudioParamEventQueue::AudioParamEventQueue(const size_t capacity)
    : capacity_(capacity), eventQueue_(capacity) {}

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

ParamChangeEvent AudioParamEventQueue::popFront() noexcept {
  ParamChangeEvent event;
  eventQueue_.popFront(event);
  return std::move(event);
}

void AudioParamEventQueue::cancelScheduledValues(double cancelTime) {
  while (!eventQueue_.isEmpty()) {
    auto &front = eventQueue_.peekBack();
    if (front.getEndTime() >= cancelTime) {
      if (front.getStartTime() >= cancelTime ||
          front.getType() == ParamChangeEventType::SET_VALUE_CURVE) {
        eventQueue_.popBack();
      }
      continue;
    }
    break;
  }
}

void AudioParamEventQueue::cancelAndHoldAtTime(
    double cancelTime,
    double &endTimeCache) {
  while (!eventQueue_.isEmpty()) {
    auto &front = eventQueue_.peekBack();
    if (front.getEndTime() >= cancelTime && front.getStartTime() > cancelTime) {
      eventQueue_.popBack();
      continue;
    }
    break;
  }

  if (eventQueue_.isEmpty()) {
    endTimeCache = cancelTime;
  } else {
    auto &back = eventQueue_.peekBackMut();
    if (back.getEndTime() > cancelTime) {
      back.setEndTime(cancelTime);
    }
  }
}

} // namespace audioapi
