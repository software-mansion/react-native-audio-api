#include "OscillatorNode.h"
#include "android/log.h"

namespace audiocontext {

    using namespace facebook::jni;

    OscillatorNode::OscillatorNode(jni::alias_ref<OscillatorNode::jhybridobject> &jThis, jlong jsContext)
            : javaObject_(make_global(jThis)), jsContext_(jsContext){}

    std::shared_ptr<OscillatorNodeHostObject> OscillatorNode::createOscillatorNodeHostObject() {
        auto oscillatorNodeWrapper = std::make_shared<OscillatorNodeWrapper>(std::shared_ptr<OscillatorNode>(this));
        return std::make_shared<OscillatorNodeHostObject>(oscillatorNodeWrapper);
    }

    void OscillatorNode::start() {
        static const auto method = javaClassLocal()->getMethod<void()>("start");
        method(javaObject_.get());
    }

    void OscillatorNode::stop() {
        static const auto method = javaClassLocal()->getMethod<void()>("stop");
        method(javaObject_.get());
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

    void OscillatorNode::connect(const AudioDestinationNode &destination) {
        const auto method = javaClassLocal()->getMethod<void(AudioDestinationNode::javaobject)>("connect");
        method(javaObject_.get(), destination.javaObject_.get());
    }
} // namespace audiocontext
