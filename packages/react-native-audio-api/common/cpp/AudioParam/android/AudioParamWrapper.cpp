#ifdef ANDROID
#include "AudioParamWrapper.h"

namespace audioapi {

AudioParamWrapper::AudioParamWrapper(AudioParam *param) : param_(param) {
  defaultValue_ = param->getDefaultValue();
  minValue_ = param->getMinValue();
  maxValue_ = param->getMaxValue();
}

AudioParamWrapper::~AudioParamWrapper() {
  param_->prepareForDeconstruction();
}

double AudioParamWrapper::getValue() {
  return param_->getValue();
}

void AudioParamWrapper::setValue(double value) {
  param_->setValue(value);
}

double AudioParamWrapper::getDefaultValue() const {
  return defaultValue_;
}

double AudioParamWrapper::getMinValue() const {
  return minValue_;
}

double AudioParamWrapper::getMaxValue() const {
  return maxValue_;
}

void AudioParamWrapper::setValueAtTime(double value, double startTime) {
  param_->setValueAtTime(value, startTime);
}

void AudioParamWrapper::linearRampToValueAtTime(double value, double endTime) {
  param_->linearRampToValueAtTime(value, endTime);
}

void AudioParamWrapper::exponentialRampToValueAtTime(
    double value,
    double endTime) {
  param_->exponentialRampToValueAtTime(value, endTime);
}
} // namespace audioapi
#endif
