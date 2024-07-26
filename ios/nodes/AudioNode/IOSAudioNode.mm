#include <IOSAudioNode.h>
#include <iostream>

namespace audiocontext {
    void IOSAudioNode::connect(std::shared_ptr<IOSAudioNode> node) {
        [audioNode_ connect:(node->audioNode_)];
    }

    void IOSAudioNode::disconnect(std::shared_ptr<IOSAudioNode> node) {
        [audioNode_ disconnect:(node->audioNode_)];
    }

    void IOSAudioNode::syncPlayerNode(AVAudioPlayerNode *node) {
        [audioNode_ syncPlayerNode:node];
    }

    void IOSAudioNode::clearPlayerNode(AVAudioPlayerNode *node) {
        [audioNode_ clearPlayerNode:node];
    }
} // namespace audiocontext
