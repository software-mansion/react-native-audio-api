#include "AudioNode.h"

namespace audiocontext {

    AudioNode::AudioNode(alias_ref<AudioNode::jhybridobject> &jThis): javaPart_(make_global(jThis)) {}

    int AudioNode::getNumberOfInputs() {
        static const auto method = javaClassLocal()->getMethod<int()>("getNumberOfInputs");
        return method(javaPart_.get());
    }

    int AudioNode::getNumberOfOutputs() {
        static const auto method = javaClassLocal()->getMethod<int()>("getNumberOfOutputs");
        return method(javaPart_.get());
    }

    void AudioNode::connect(const std::shared_ptr<AudioNode> &node) {
        static const auto method = javaClassLocal()->getMethod<void(AudioNode::javaobject)>(
                "connect");
        method(javaPart_.get(), node->javaPart_.get());
    }

    void AudioNode::disconnect(const std::shared_ptr<AudioNode> &node) {
        static const auto method = javaClassLocal()->getMethod<void(AudioNode::javaobject)>(
                "disconnect");
        method(javaPart_.get(), node->javaPart_.get());
    }
}
