#include "AudioParam.h"

namespace audioapi {

AudioParam::AudioParam(float defaultValue, float minValue, float maxValue)
    : value_(defaultValue),
      defaultValue_(defaultValue),
      minValue_(minValue),
      maxValue_(maxValue) {}

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
  value_ = value;
}

void AudioParam::setValueAtTime(float value, float startTime) {
  // TODO: Implement this
}

void AudioParam::linearRampToValueAtTime(float value, float endTime) {
  // TODO: Implement this
}

void AudioParam::exponentialRampToValueAtTime(float value, float endTime) {
  // TODO: Implement this
}

} // namespace audioapi
