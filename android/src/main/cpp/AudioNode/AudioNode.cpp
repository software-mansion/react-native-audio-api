#include "AudioNode.h"

namespace audiocontext {

    AudioNode::AudioNode(alias_ref<AudioNode::jhybridobject> &jThis): javaObject_(make_global(jThis)) {}

    int AudioNode::getNumberOfInputs() {
        static const auto method = javaClassLocal()->getMethod<int()>("getNumberOfInputs");
        return method(javaObject_.get());
    }

    int AudioNode::getNumberOfOutputs() {
        static const auto method = javaClassLocal()->getMethod<int()>("getNumberOfOutputs");
        return method(javaObject_.get());
    }

    void AudioNode::connect(const std::shared_ptr<AudioNode> &node) {
        static const auto method = javaClassLocal()->getMethod<void(AudioNode::javaobject)>(
                "connect");
        method(javaObject_.get(), node->javaObject_.get());
    }

    void AudioNode::disconnect(const std::shared_ptr<AudioNode> &node) {
        static const auto method = javaClassLocal()->getMethod<void(AudioNode::javaobject)>(
                "disconnect");
        method(javaObject_.get(), node->javaObject_.get());
    }
}
