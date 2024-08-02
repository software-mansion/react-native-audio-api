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

    double AudioParamWrapper::getDefaultValue() const {
        throw std::runtime_error("[AudioParamHostObject] getDefaultValue not implemented yet!");
    }

    double AudioParamWrapper::getMinValue() const {
        throw std::runtime_error("[AudioParamHostObject] getMinValue not implemented yet!");
    }

    double AudioParamWrapper::getMaxValue() const {
        throw std::runtime_error("[AudioParamHostObject] getMaxValue not implemented yet!");
    }
} // namespace audiocontext
#endif
