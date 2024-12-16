#include "AudioParam.h"

#include "AudioUtils.h"
#include "BaseAudioContext.h"

namespace audioapi {

AudioParam::AudioParam(
    BaseAudioContext *context,
    float defaultValue,
    float minValue,
    float maxValue)
    : value_(defaultValue),
      defaultValue_(defaultValue),
      minValue_(minValue),
      maxValue_(maxValue),
      context_(context) {
  startTime_ = 0;
  endTime_ = 0;
  startValue_ = value_;
  endValue_ = value_;
  calculateValue_ = [this](double, double, float, float, double) {
    return value_;
  };
}

float AudioParam::getValue() const {
  return value_;
}

float AudioParam::getDefaultValue() const {
  return defaultValue_;
}

float AudioParam::getMinValue() const {
  return minValue_;
}

float AudioParam::getMaxValue() const {
  return maxValue_;
}

void AudioParam::setValue(float value) {
  value_ = std::clamp(value, minValue_, maxValue_);
}

float AudioParam::getValueAtTime(double time) {
  if (endTime_ < time) {
    if (!eventsQueue_.empty()) {
      auto event = eventsQueue_.begin();
      startTime_ = event->getStartTime();
      endTime_ = event->getEndTime();
      startValue_ = event->getStartValue();
      endValue_ = event->getEndValue();
      calculateValue_ = event->getCalculateValue();
      eventsQueue_.erase(eventsQueue_.begin());
    }
  }

  setValue(calculateValue_(startTime_, endTime_, startValue_, endValue_, time));

  return value_;
}

void AudioParam::setValueAtTime(float value, double startTime) {
  // if the start time is less than the end time of last event in the queue,
  // then there is no need to schedule any events.
  if (startTime <= getQueueEndTime()) {
    return;
  }

  auto calculateValue = [](double startTime,
                           double,
                           float startValue,
                           float endValue,
                           double time) {
    if (time < startTime) {
      return startValue;
    }

    return endValue;
  };

  auto event = ParamChangeEvent(
      startTime,
      startTime,
      value,
      value,
      calculateValue,
      ParamChangeEventType::START_TIME_EVENT);
  updateQueue(event);
}

void AudioParam::linearRampToValueAtTime(float value, double endTime) {
  // if the end time is less than the end time of last event in the queue, then
  // there is no need to schedule any events.
  if (endTime <= getQueueEndTime()) {
    return;
  }

  auto calculateValue = [](double startTime,
                           double endTime,
                           float startValue,
                           float endValue,
                           double time) {
    if (time < startTime) {
      return startValue;
    }

    if (time < endTime) {
      return static_cast<float>(
          startValue +
          (endValue - startValue) * (time - startTime) / (endTime - startTime));
    }

    return endValue;
  };

  auto event = ParamChangeEvent(
      getQueueEndTime(),
      endTime,
      getQueueEndValue(),
      value,
      calculateValue,
      ParamChangeEventType::END_TIME_EVENT);
  updateQueue(event);
}

void AudioParam::exponentialRampToValueAtTime(float value, double endTime) {
  // if the end time is less than the end time of last event in the queue, then
  // there is no need to schedule any events.
  if (endTime <= getQueueEndTime()) {
    return;
  }

  auto calculateValue = [](double startTime,
                           double endTime,
                           float startValue,
                           float endValue,
                           double time) {
    if (time < startTime) {
      return startValue;
    }

    if (time < endTime) {
      return static_cast<float>(
          startValue *
          pow(endValue / startValue,
              (time - startTime) / (endTime - startTime)));
    }

    return endValue;
  };

  auto event = ParamChangeEvent(
      getQueueEndTime(),
      endTime,
      getQueueEndValue(),
      value,
      calculateValue,
      ParamChangeEventType::END_TIME_EVENT);
  updateQueue(event);
}

void AudioParam::setTargetAtTime(
    float target,
    double startTime,
    double timeConstant) {
  // if the start time is less than the end time of last event in the queue,
  // then there is no need to schedule any events.
  if (startTime <= getQueueEndTime()) {
    return;
  }

  auto calculateValue =
      [timeConstant, target](
          double startTime, double, float startValue, float, double time) {
        if (time < startTime) {
          return startValue;
        }

        return static_cast<float>(
            target +
            (startValue - target) * exp(-(time - startTime) / timeConstant));
      };

  auto event = ParamChangeEvent(
      startTime,
      startTime,
      getQueueEndValue(),
      getQueueEndValue(),
      calculateValue,
      ParamChangeEventType::START_TIME_EVENT);
  updateQueue(event);
}

void AudioParam::setValueCurveAtTime(
    const float *values,
    int length,
    double startTime,
    double duration) {
  // if the start time is less than the end time of last event in the queue,
  // then there is no need to schedule any events.
  if (startTime <= getQueueEndTime()) {
    return;
  }

  auto calculateValue = [&values, length](
                            double startTime,
                            double endTime,
                            float startValue,
                            float endValue,
                            double time) {
    if (time < startTime) {
      return startValue;
    }

    if (time < endTime) {
      auto k = static_cast<int>(std::floor(
          (length - 1) / (endTime - startTime) * (time - startTime)));
      auto factor = static_cast<float>(
          k - (time - startTime) * (length - 1) / (endTime - startTime));

      return AudioUtils::linearInterpolate(values, k, k + 1, factor);
    }

    return endValue;
  };

  auto event = ParamChangeEvent(
      startTime,
      startTime + duration,
      getQueueEndValue(),
      values[length - 1],
      calculateValue,
      ParamChangeEventType::START_END_TIME_EVENT);
  updateQueue(event);
}

void AudioParam::cancelScheduledValues(double cancelTime) {
  bool shouldErase;

  for (auto &event : eventsQueue_) {
    shouldErase = false;
    if (event.getStartTime() >= cancelTime) {
      shouldErase = true;
    }

    if (event.getType() == ParamChangeEventType::START_END_TIME_EVENT) {
      if (event.getStartTime() < cancelTime &&
          event.getEndTime() > cancelTime) {
        shouldErase = false;
      }
    }

    if (shouldErase) {
      eventsQueue_.erase(event);
    }
  }
}

void AudioParam::cancelAndHoldAtTime(double cancelTime) {
  for (auto &event : eventsQueue_) {
    if (event.getStartTime() >= cancelTime) {
      eventsQueue_.erase(event);
    }
  }

  if (eventsQueue_.empty()) {
    endTime_ = cancelTime;
  }

  if (!eventsQueue_.empty()) {
    auto lastEvent = eventsQueue_.rbegin();
    if (lastEvent->getEndTime() > cancelTime) {
      auto event = new ParamChangeEvent(
          lastEvent->getStartTime(),
          cancelTime,
          lastEvent->getStartValue(),
          lastEvent->getEndValue(),
          lastEvent->getCalculateValue(),
          lastEvent->getType());
      eventsQueue_.erase(*lastEvent);
      eventsQueue_.insert(*event);
    }
  }
}

double AudioParam::getQueueEndTime() {
  if (eventsQueue_.empty()) {
    return endTime_;
  }

  return eventsQueue_.rbegin()->getEndTime();
}

float AudioParam::getQueueEndValue() {
  if (eventsQueue_.empty()) {
    return this->endValue_;
  }

  return eventsQueue_.rbegin()->getEndValue();
}

void AudioParam::updateQueue(ParamChangeEvent &event) {
  auto prev = eventsQueue_.rbegin();

  if (prev == eventsQueue_.rend()) {
    eventsQueue_.insert(event);
    return;
  }

  if (prev->getType() == ParamChangeEventType::START_TIME_EVENT) {
    auto prevCalculateValue = prev->getCalculateValue();
    auto prevEndValue = prevCalculateValue(
        prev->getStartTime(),
        prev->getEndTime(),
        prev->getStartValue(),
        prev->getEndValue(),
        event.getStartTime());
    auto prevEvent = new ParamChangeEvent(
        prev->getStartTime(),
        event.getStartTime(),
        prev->getStartValue(),
        prevEndValue,
        prev->getCalculateValue(),
        ParamChangeEventType::START_TIME_EVENT);
    eventsQueue_.erase(*prev);
    eventsQueue_.insert(*prevEvent);
  }

  event.setStartValue(prev->getEndValue());
  eventsQueue_.insert(event);
}

} // namespace audioapi
