#include <IOSAudioNode.h>

namespace audiocontext {
    void IOSAudioNode::connect(std::shared_ptr<IOSAudioNode> node) {
        [AudioNode_ connect:(node->AudioNode_)];
    }

    void IOSAudioNode::disconnect(std::shared_ptr<IOSAudioNode> node) {
        [AudioNode_ disconnect:(node->AudioNode_)];
    }

    void IOSAudioNode::disconnectAttachedNode(std::shared_ptr<IOSAudioNode> node) {
        [AudioNode_ disconnectAttachedNode:(node->AudioNode_)];
    }
} // namespace audiocontext
