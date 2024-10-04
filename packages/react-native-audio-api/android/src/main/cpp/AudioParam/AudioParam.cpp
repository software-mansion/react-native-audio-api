#include "AudioParam.h"
#include "AudioContext.h"

namespace audioapi {

AudioParam::AudioParam(
    AudioContext *context,
    float defaultValue,
    float minValue,
    float maxValue)
    : value_(defaultValue),
      defaultValue_(defaultValue),
      minValue_(minValue),
      maxValue_(maxValue),
      context_(context),
      currentChange_(nullptr),
      changesQueue_() {}

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
    if (!currentChange_ || currentChange_->getEndTime() < time) {
      auto change = *changesQueue_.begin();
      currentChange_ = &change;
      changesQueue_.erase(changesQueue_.begin());
    }
  }

  if (currentChange_ && currentChange_->getStartTime() <= time) {
    value_ = currentChange_->getValueAtTime(time);
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
    return startValue +
        (endValue - startValue) * (time - startTime) / (endTime - startTime);
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
    return startValue *
        pow(endValue / startValue, (time - startTime) / (endTime - startTime));
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
