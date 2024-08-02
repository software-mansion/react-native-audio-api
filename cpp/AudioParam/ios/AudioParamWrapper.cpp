#ifndef ANDROID
#include "AudioParamWrapper.h"

namespace audiocontext
{
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
} // namespace audiocontext
#endif

