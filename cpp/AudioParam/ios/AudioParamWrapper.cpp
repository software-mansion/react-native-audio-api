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
} // namespace audiocontext
#endif
