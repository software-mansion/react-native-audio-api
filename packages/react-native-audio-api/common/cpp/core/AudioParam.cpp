#include "AudioParam.h"
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
      context_(context),
      changesQueue_() {
  startTime_ = 0;
  endTime_ = 0;
  startValue_ = 0;
  endValue_ = 0;
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
  checkValue(value);
  value_ = value;
}

float AudioParam::getValueAtTime(double time) {
  if (!changesQueue_.empty()) {
    if (endTime_ < time) {
      auto change = *changesQueue_.begin();
      startTime_ = change.getStartTime();
      endTime_ = change.getEndTime();
      startValue_ = change.getStartValue();
      endValue_ = change.getEndValue();
      calculateValue_ = change.getCalculateValue();
      changesQueue_.erase(changesQueue_.begin());
    }
  }

  if (startTime_ <= time) {
    value_ =
        calculateValue_(startTime_, endTime_, startValue_, endValue_, time);
  }

  return value_;
}

void AudioParam::setValueAtTime(float value, double time) {
  checkValue(value);
  auto calculateValue = [](double, double, float, float endValue, double) {
    return endValue;
  };

  auto paramChange = ParamChange(time, time, value, value, calculateValue);
  changesQueue_.insert(paramChange);
}

void AudioParam::linearRampToValueAtTime(float value, double time) {
  checkValue(value);
  auto calculateValue = [](double startTime,
                           double endTime,
                           float startValue,
                           float endValue,
                           double time) {
    return time >= endTime ? endValue
                           : startValue +
            (endValue - startValue) * (time - startTime) /
                (endTime - startTime);
  };

  auto paramChange =
      ParamChange(getStartTime(), time, getStartValue(), value, calculateValue);
  changesQueue_.emplace(paramChange);
}

void AudioParam::exponentialRampToValueAtTime(float value, double time) {
  checkValue(value);
  auto calculateValue = [](double startTime,
                           double endTime,
                           float startValue,
                           float endValue,
                           double time) {
    return time >= endTime ? endValue
                           : startValue *
            pow(endValue / startValue,
                (time - startTime) / (endTime - startTime));
  };

  auto paramChange =
      ParamChange(getStartTime(), time, getStartValue(), value, calculateValue);
  changesQueue_.emplace(paramChange);
}

void AudioParam::checkValue(float value) const {
  if (value < minValue_ || value > maxValue_) {
    throw std::invalid_argument("Value out of range");
  }
}

double AudioParam::getStartTime() {
  if (changesQueue_.empty()) {
    return context_->getCurrentTime();
  }

  return changesQueue_.rbegin()->getEndTime();
}

float AudioParam::getStartValue() {
  if (changesQueue_.empty()) {
    return this->value_;
  }

  return changesQueue_.rbegin()->getEndValue();
}

} // namespace audioapi
