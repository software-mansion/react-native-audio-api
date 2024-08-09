#include "BiquadFilterNode.h"

namespace audiocontext{

    using namespace facebook::jni;

    std::shared_ptr<AudioParam> BiquadFilterNode::getFrequencyParam() {
        static const auto method = javaClassLocal()->getMethod<AudioParam()>("getFrequency");
        auto frequency = method(javaPart_.get());
        auto frequencyCppInstance = frequency->cthis();

        return std::shared_ptr<AudioParam>(frequencyCppInstance);
    }

    std::shared_ptr<AudioParam> BiquadFilterNode::getDetuneParam() {
        static const auto method = javaClassLocal()->getMethod<AudioParam()>("getDetune");
        auto detune = method(javaPart_.get());
        auto detuneCppInstance = detune->cthis();

        return std::shared_ptr<AudioParam>(detuneCppInstance);
    }

    std::shared_ptr<AudioParam> BiquadFilterNode::getQParam() {
        static const auto method = javaClassLocal()->getMethod<AudioParam()>("getQ");
        auto Q = method(javaPart_.get());
        auto QCppInstance = Q->cthis();

        return std::shared_ptr<AudioParam>(QCppInstance);
    }

    std::shared_ptr<AudioParam> BiquadFilterNode::getGainParam() {
        static const auto method = javaClassLocal()->getMethod<AudioParam()>("getGain");
        auto gain = method(javaPart_.get());
        auto gainCppInstance = gain->cthis();

        return std::shared_ptr<AudioParam>(gainCppInstance);
    }

    std::string BiquadFilterNode::getFilterType() {
        static const auto method = javaClassLocal()->getMethod<JString()>("getFilterType");
        return method(javaPart_.get())->toStdString();
    }

    void BiquadFilterNode::setFilterType(const std::string &filterType) {
        static const auto method = javaClassLocal()->getMethod<void(JString)>("setFilterType");
        method(javaPart_.get(), *make_jstring(filterType));
    }
}
