#include <IOSGainNode.h>

namespace audiocontext {

	IOSGainNode::IOSGainNode(std::shared_ptr<IOSAudioContext> context) {
		audioNode_ = gainNode_ = [[GainNode alloc] init:context->audioContext_];
	}

    void IOSGainNode::changeGain(const double gain) const {
        [gainNode_ changeGain:gain];
    }

    double IOSGainNode::getGain() const {
        return [gainNode_ getGain];
    }
} // namespace audiocontext
