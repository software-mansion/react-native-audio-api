#include <IOSGainNode.h>

namespace audiocontext {

	IOSGainNode::IOSGainNode() {
		AudioNode_ = gainNode_ = [[GainNode alloc] init];
	}

    void IOSGainNode::changeGain(const double gain) const {
        [gainNode_ changeGain:gain];
    }

    double IOSGainNode::getGain() const {
        return [gainNode_ getGain];
    }
} // namespace audiocontext
