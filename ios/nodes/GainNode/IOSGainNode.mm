#include <IOSGainNode.h>

namespace audiocontext {

	IOSGainNode::IOSGainNode() {
		gainNode_ = [[GainNode alloc] init];
        AudioNode_ = gainNode_;
	}

    void IOSGainNode::changeGain(const double gain) const {
        [gainNode_ changeGain:gain];
    }
} // namespace audiocontext
