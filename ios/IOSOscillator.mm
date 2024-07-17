#include <IOSOscillator.h>


namespace audiocontext {

	IOSOscillator::IOSOscillator(const float frequency) {
		OscillatorNode_ = [[OscillatorNode alloc] init:frequency];
	}

	void IOSOscillator::start() const {
		[OscillatorNode_ start];
	}

	void IOSOscillator::stop() const {
		[OscillatorNode_ stop];
	}

	void IOSOscillator::changeFrequency(const float frequency) const {
		[OscillatorNode_ changeFrequency:frequency];
	}

} // namespace audiocontext
