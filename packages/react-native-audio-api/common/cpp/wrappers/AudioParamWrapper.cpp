#ifdef ANDROID
#include "AudioParamWrapper.h"

namespace audioapi {

AudioParamWrapper::AudioParamWrapper(const std::shared_ptr<AudioParam> &param)
    : param_(param) {}

float AudioParamWrapper::getValue() const {
  return param_->getValue();
}

void AudioParamWrapper::setValue(float value) const {
  param_->setValue(value);
}

float AudioParamWrapper::getDefaultValue() const {
  return param_->getDefaultValue();
}

float AudioParamWrapper::getMinValue() const {
  return param_->getMinValue();
}

float AudioParamWrapper::getMaxValue() const {
  return param_->getMaxValue();
}

void AudioParamWrapper::setValueAtTime(float value, double startTime) const {
  param_->setValueAtTime(value, startTime);
}

void AudioParamWrapper::linearRampToValueAtTime(float value, double endTime)
    const {
  param_->linearRampToValueAtTime(value, endTime);
}

void AudioParamWrapper::exponentialRampToValueAtTime(
    float value,
    double endTime) const {
  param_->exponentialRampToValueAtTime(value, endTime);
}
} // namespace audioapi
#endif
