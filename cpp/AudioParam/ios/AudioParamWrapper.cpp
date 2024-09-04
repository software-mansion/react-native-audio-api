#ifndef ANDROID
#include "AudioParamWrapper.h"

namespace audiocontext {
double AudioParamWrapper::getValue() {
  return param_->getValue();
}

void AudioParamWrapper::setValue(double value) {
  param_->setValue(value);
}

double AudioParamWrapper::getMaxValue() const {
  return param_->getMaxValue();
}

double AudioParamWrapper::getMinValue() const {
  return param_->getMinValue();
}

double AudioParamWrapper::getDefaultValue() const {
  return param_->getDefaultValue();
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
} // namespace audiocontext
#endif
