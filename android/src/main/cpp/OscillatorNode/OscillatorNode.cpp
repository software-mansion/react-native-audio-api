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

    std::shared_ptr<AudioParam> OscillatorNode::getFrequencyParam() {
        static const auto method = javaClassLocal()->getMethod<AudioParam()>("getFrequency");
        auto frquency = method(javaObject_.get());
        auto frquencyCppInstance = frquency->cthis();

        return std::shared_ptr<AudioParam>(frquencyCppInstance);
    }

    std::shared_ptr<AudioParam> OscillatorNode::getDetuneParam() {
        static const auto method = javaClassLocal()->getMethod<AudioParam()>("getDetune");
        auto detune = method(javaObject_.get());
        auto detuneCppInstance = detune->cthis();

        return std::shared_ptr<AudioParam>(detuneCppInstance);
    }

    std::string OscillatorNode::getWaveType() {
        static const auto method = javaClassLocal()->getMethod<JString()>("getWaveType");
        return method(javaObject_.get())->toStdString();
    }

    void OscillatorNode::setWaveType(const std::string& waveType) {
        static const auto method = javaClassLocal()->getMethod<void(JString)>("setWaveType");
        method(javaObject_.get(), *make_jstring(waveType));

    }
} // namespace audiocontext
