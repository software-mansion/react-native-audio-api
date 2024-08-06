#include <IOSAudioNode.h>

namespace audiocontext {
    IOSAudioNode::~IOSAudioNode() {
        [audioNode_ clean];
        audioNode_ = nil;
    }

    void IOSAudioNode::connect(std::shared_ptr<IOSAudioNode> node) {
        [audioNode_ connect:(node->audioNode_)];
    }

    void IOSAudioNode::disconnect(std::shared_ptr<IOSAudioNode> node) {
        [audioNode_ disconnect:(node->audioNode_)];
    }

    int IOSAudioNode::getNumberOfInputs() {
        return [audioNode_ getNumberOfInputs];
    }

    int IOSAudioNode::getNumberOfOutputs() {
        return [audioNode_ getNumberOfOutputs];
    }
} // namespace audiocontext
