#include "AudioParam.h"

namespace audiocontext {

    AudioParam::AudioParam(alias_ref<AudioParam::jhybridobject> &jThis): javaPart_(make_global(jThis)) {}

    double AudioParam::getValue() {
        static const auto method = javaClassLocal()->getMethod<double()>("getValue");
        return method(javaPart_.get());
    }

    void AudioParam::setValue(double value) {
        static const auto method = javaClassLocal()->getMethod<void(double)>("setValue");
        method(javaPart_.get(), value);
    }

    double AudioParam::getDefaultValue() {
        static const auto method = javaClassLocal()->getMethod<double()>("getDefaultValue");
        return method(javaPart_.get());
    }

    double AudioParam::getMinValue() {
        static const auto method = javaClassLocal()->getMethod<double()>("getMinValue");
        return method(javaPart_.get());
    }

    double AudioParam::getMaxValue() {
        static const auto method = javaClassLocal()->getMethod<double()>("getMaxValue");
        return method(javaPart_.get());
    }

    void AudioParam::setValueAtTime(double value, double startTime) {
        static const auto method = javaClassLocal()->getMethod<void(double, double)>("setValueAtTime");
        method(javaPart_.get(), value, startTime);
    }

    void AudioParam::linearRampToValueAtTime(double value, double endTime) {
        static const auto method = javaClassLocal()->getMethod<void(double, double)>("linearRampToValueAtTime");
        method(javaPart_.get(), value, endTime);
    }

    void AudioParam::exponentialRampToValueAtTime(double value, double endTime) {
        static const auto method = javaClassLocal()->getMethod<void(double, double)>("exponentialRampToValueAtTime");
        method(javaPart_.get(), value, endTime);
    }
}
