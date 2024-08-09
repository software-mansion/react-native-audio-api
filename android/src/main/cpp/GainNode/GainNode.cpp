#include "GainNode.h"

namespace audiocontext{

    using namespace facebook::jni;

    std::shared_ptr<AudioParam> GainNode::getGainParam() {
        static const auto method = javaClassLocal()->getMethod<AudioParam()>("getGain");
        auto gain = method(javaPart_.get());
        auto gainCppInstance = gain->cthis();

        return std::shared_ptr<AudioParam>(gainCppInstance);
    }
}
