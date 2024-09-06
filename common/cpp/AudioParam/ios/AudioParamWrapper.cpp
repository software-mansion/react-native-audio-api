#ifndef ANDROID
#include "AudioParamWrapper.h"

namespace audioapi {
AudioParamWrapper::AudioParamWrapper(std::shared_ptr<IOSAudioParam> param) {
    param_ = param;
    defaultValue_ = param_->getDefaultValue();
    minValue_ = param_->getMinValue();
    maxValue_ = param_->getMaxValue();
}

double AudioParamWrapper::getValue() {
  return param_->getValue();
}

void AudioParamWrapper::setValue(double value) {
  param_->setValue(value);
}

double AudioParamWrapper::getMaxValue() const {
    return maxValue_;
}

double AudioParamWrapper::getMinValue() const {
    return minValue_;
}

double AudioParamWrapper::getDefaultValue() const {
    return defaultValue_;
}

void AudioParamWrapper::setValueAtTime(double value, double startTime) {
  return param_->setValueAtTime(value, startTime);
}

void AudioParamWrapper::linearRampToValueAtTime(double value, double endTime) {
  return param_->linearRampToValueAtTime(value, endTime);
}

void AudioParamWrapper::exponentialRampToValueAtTime(
    double value,
    double endTime) {
  return param_->exponentialRampToValueAtTime(value, endTime);
}
} // namespace audioapi
#endif
