#include "GainNode.h"

namespace audiocontext{

    using namespace facebook::jni;

    double GainNode::getGain(){
        static const auto method = javaClassLocal()->getMethod<jdouble()>("getGain");
        return method(javaObject_.get());
    }

    void GainNode::setGain(double gain){
        static const auto method = javaClassLocal()->getMethod<void(jdouble)>("setGain");
        method(javaObject_.get(), gain);
    }
}
