#include <IOSAudioNode.h>
#include <iostream>

namespace audiocontext {
    void IOSAudioNode::connect(std::shared_ptr<IOSAudioNode> node) {
        [audioNode_ connect:(node->audioNode_)];
    }

    void IOSAudioNode::disconnect(std::shared_ptr<IOSAudioNode> node) {
        [audioNode_ disconnect:(node->audioNode_)];
    }

    void IOSAudioNode::addConnectedTo(AVAudioPlayerNode *node) {
        [audioNode_ addConnectedTo:node];
    }

    void IOSAudioNode::removeConnectedTo(AVAudioPlayerNode *node) {
        [audioNode_ removeConnectedTo:node];
    }
} // namespace audiocontext
