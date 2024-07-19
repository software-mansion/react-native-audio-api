#include "AudioDestinationNode.h"

namespace audiocontext {

    using namespace facebook::jni;

    AudioDestinationNode::AudioDestinationNode(jni::alias_ref<AudioDestinationNode::jhybridobject> &jThis,
                               jlong jsContext): javaObject_(make_global(jThis)), jsContext_(jsContext) {}

    std::shared_ptr<AudioDestinationNodeHostObject> AudioDestinationNode::createAudioDestinationHostObject() {
        auto audioDestinationNodeWrapper = std::make_shared<AudioDestinationNodeWrapper>(std::shared_ptr<AudioDestinationNode>(this));
        return std::make_shared<AudioDestinationNodeHostObject>(audioDestinationNodeWrapper);
    }
} // namespace audiocontext
