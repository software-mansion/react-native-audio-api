#include <IOSAudioNode.h>

namespace audiocontext {
    void IOSAudioNode::connect(std::shared_ptr<IOSAudioNode> node) {
        [audioNode_ connect:(node->audioNode_)];
    }

    void IOSAudioNode::disconnect(std::shared_ptr<IOSAudioNode> node) {
        [audioNode_ disconnect:(node->audioNode_)];
    }
} // namespace audiocontext
