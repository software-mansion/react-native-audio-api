#include "AudioBufferSourceNode.h"

namespace audiocontext {

    using namespace facebook::jni;

    void AudioBufferSourceNode::start(double time) {
        static const auto method = javaClassLocal()->getMethod<void(jdouble)>("start");
        method(javaPart_.get(), time);
    }

    void AudioBufferSourceNode::stop(double time) {
        static const auto method = javaClassLocal()->getMethod<void(jdouble)>("stop");
        method(javaPart_.get(), time);
    }

    bool AudioBufferSourceNode::getLoop() {
        static const auto method = javaClassLocal()->getMethod<jboolean()>("getLoop");
        return method(javaPart_.get());
    }

    void AudioBufferSourceNode::setLoop(bool loop) {
        static const auto method = javaClassLocal()->getMethod<void(jboolean)>("setLoop");
        method(javaPart_.get(), loop);
    }

    AudioBuffer* AudioBufferSourceNode::getBuffer() {
        static const auto method = javaClassLocal()->getMethod<AudioBuffer()>("getBuffer");
        auto buffer = method(javaPart_.get());

        return buffer->cthis();
    }
} // namespace audiocontext
