#include <IOSGainNode.h>

namespace audiocontext {

    IOSGainNode::IOSGainNode(std::shared_ptr<IOSAudioContext> context) {
        audioNode_ = gainNode_ = [[GainNode alloc] init:context->audioContext_];
    }

    void IOSGainNode::setGain(const float gain) const {
        [gainNode_ setGain:gain];
    }

    float IOSGainNode::getGain() const {
        return [gainNode_ getGain];
    }
} // namespace audiocontext
