#include "OscillatorNode.h"

namespace audiocontext {

    using namespace facebook::jni;

    void OscillatorNode::start(double time) {
        static const auto method = javaClassLocal()->getMethod<void(jdouble)>("start");
        method(javaObject_.get(), time);
    }

    void OscillatorNode::stop(double time) {
        static const auto method = javaClassLocal()->getMethod<void(jdouble)>("stop");
        method(javaObject_.get(), time);
    }

    double OscillatorNode::getFrequency() {
        static const auto method = javaClassLocal()->getMethod<jdouble()>("getFrequency");
        return method(javaObject_.get());
    }

    double OscillatorNode::getDetune() {
        static const auto method = javaClassLocal()->getMethod<jdouble()>("getDetune");
        return method(javaObject_.get());
    }

    std::string OscillatorNode::getWaveType() {
        static const auto method = javaClassLocal()->getMethod<JString()>("getWaveType");
        return method(javaObject_.get())->toStdString();
    }

    void OscillatorNode::setFrequency(double frequency) {
        static const auto method = javaClassLocal()->getMethod<void(jdouble)>("setFrequency");
        method(javaObject_.get(), frequency);
    }

    void OscillatorNode::setDetune(double detune) {
        static const auto method = javaClassLocal()->getMethod<void(jdouble)>("setDetune");
        method(javaObject_.get(), detune);
    }

    void OscillatorNode::setWaveType(const std::string& waveType) {
        static const auto method = javaClassLocal()->getMethod<void(JString)>("setWaveType");
        method(javaObject_.get(), *make_jstring(waveType));

    }
} // namespace audiocontext
