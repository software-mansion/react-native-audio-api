#include "StereoPannerNode.h"

namespace audiocontext{

    using namespace facebook::jni;

    double StereoPannerNode::getPan(){
        static const auto method = javaClassLocal()->getMethod<jdouble()>("getPan");
        return method(javaObject_.get());
    }

    void StereoPannerNode::setPan(double pan){
        static const auto method = javaClassLocal()->getMethod<void(jdouble)>("setPan");
        method(javaObject_.get(), pan);
    }
}
