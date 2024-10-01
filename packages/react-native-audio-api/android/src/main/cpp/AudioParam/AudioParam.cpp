#include "AudioParam.h"

namespace audioapi {

AudioParam::AudioParam(double defaultValue, double minValue, double maxValue)
    : value_(defaultValue), defaultValue_(defaultValue), minValue_(minValue),
      maxValue_(maxValue) {}

double AudioParam::getValue() const { return value_; }

double AudioParam::getDefaultValue() const { return defaultValue_; }

double AudioParam::getMinValue() const { return minValue_; }

double AudioParam::getMaxValue() const { return maxValue_; }

void AudioParam::setValue(double value) { value_ = value; }

void AudioParam::setValueAtTime(double value, double startTime) {
  //TODO: Implement this
}

void AudioParam::linearRampToValueAtTime(double value, double endTime) {
  //TODO: Implement this
}

void AudioParam::exponentialRampToValueAtTime(double value, double endTime) {
  //TODO: Implement this
}

} // namespace audioapi
