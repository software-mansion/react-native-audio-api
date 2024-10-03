#include "AudioParam.h"

namespace audioapi {

AudioParam::AudioParam(float defaultValue, float minValue, float maxValue)
    : value_(defaultValue),
      defaultValue_(defaultValue),
      minValue_(minValue),
      maxValue_(maxValue) {
    changesQueue_ = {};
    currentChange_ = nullptr;
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
  value_ = checkValue(value);
}

float AudioParam::getValueAtTime(double time) {
    if (!changesQueue_.empty()) {
        if (!currentChange_ || currentChange_->getEndTime() < time) {
            auto &nextChange = changesQueue_.top();
            currentChange_ = const_cast<ParamChange*>(&nextChange);
            changesQueue_.pop();
        }
    }

    if (currentChange_ && currentChange_->getStartTime() <= time) {
        value_ = currentChange_->getValueAtTime(time);
    }

    return value_;
}

void AudioParam::setValueAtTime(float value, double startTime) {
  auto endValue = checkValue(value);
    auto calculateValue = [](double, double, float, float endValue, double) {
        return endValue;
    };

//    auto paramChange = ParamChange(time, time, value, value, calculateValue);
//    changesQueue_.emplace(paramChange);
}

void AudioParam::linearRampToValueAtTime(float value, double endTime) {
    auto endValue = checkValue(value);
    auto calculateValue = [](double startTime, double endTime, float startValue, float endValue, double time) {
        return startValue + (endValue - startValue) * (time - startTime) / (endTime - startTime);
    };

//    auto paramChange = ParamChange(time, time, endValue, endValue, calculateValue);
//    changesQueue_.emplace(paramChange);
}

void AudioParam::exponentialRampToValueAtTime(float value, double endTime) {
  // TODO: Implement this
}

float AudioParam::checkValue(float value) const {
    if (value < minValue_ || value > maxValue_) {
        return defaultValue_;
    }

    return value;
}

double AudioParam::getStartTime() {
    //TODO: Implement this
    if (changesQueue_.empty()) {
        //return context.getCurrentTime();
    }

    return 0.0;
}

} // namespace audioapi
