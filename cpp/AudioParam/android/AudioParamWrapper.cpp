#ifdef ANDROID
#include "AudioParamWrapper.h"

namespace audiocontext{

    AudioParamWrapper::AudioParamWrapper(const std::shared_ptr<AudioParam> &param) : param_(param) {
        defaultValue_ = param->getDefaultValue();
        minValue_ = param->getMinValue();
        maxValue_ = param->getMaxValue();
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
}
#endif