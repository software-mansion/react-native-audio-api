#include "AudioParam.h"

namespace audiocontext {

    AudioParam::AudioParam(alias_ref<AudioParam::jhybridobject> &jThis): javaObject_(make_global(jThis)) {}

    double AudioParam::getValue() {
        static const auto method = javaClassLocal()->getMethod<double()>("getValue");
        return method(javaObject_.get());
    }

    void AudioParam::setValue(double value) {
        static const auto method = javaClassLocal()->getMethod<void(double)>("setValue");
        method(javaObject_.get(), value);
    }
}
