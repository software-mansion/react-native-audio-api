#include "StereoPannerNode.h"

namespace audiocontext{

    using namespace facebook::jni;

    std::shared_ptr<AudioParam> StereoPannerNode::getPanParam() {
        static const auto method = javaClassLocal()->getMethod<AudioParam()>("getPan");
        auto pan = method(javaObject_.get());
        auto panCppInstance = pan->cthis();

        return std::shared_ptr<AudioParam>(panCppInstance);
    }
}
