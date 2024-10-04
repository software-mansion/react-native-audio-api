#ifndef ANDROID
#include "AudioParamWrapper.h"

namespace audioapi {
AudioParamWrapper::AudioParamWrapper(const std::shared_ptr<IOSAudioParam> &param)
    : param_(param) {}

float AudioParamWrapper::getValue() const {
  return param_->getValue();
}

void AudioParamWrapper::setValue(float value) const {
  param_->setValue(value);
}

float AudioParamWrapper::getMaxValue() const {
  return param_->getMaxValue();
}

float AudioParamWrapper::getMinValue() const {
    return param_->getMinValue();
}

float AudioParamWrapper::getDefaultValue() const {
    return param_->getDefaultValue();
}

void AudioParamWrapper::setValueAtTime(float value, double startTime) const {
  return param_->setValueAtTime(value, startTime);
}

void AudioParamWrapper::linearRampToValueAtTime(float value, double endTime) const {
  return param_->linearRampToValueAtTime(value, endTime);
}

void AudioParamWrapper::exponentialRampToValueAtTime(
    float value,
    double endTime) const {
  return param_->exponentialRampToValueAtTime(value, endTime);
}
} // namespace audioapi
#endif
