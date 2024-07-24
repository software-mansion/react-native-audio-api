#include <IOSAudioNode.h>

namespace audiocontext {
    void IOSAudioNode::connect(std::shared_ptr<IOSAudioNode> node) {
        [AudioNode_ connect:(node->AudioNode_)];
    }

    void IOSAudioNode::disconnect() {
        [AudioNode_ disconnect];
    }
} // namespace audiocontext
