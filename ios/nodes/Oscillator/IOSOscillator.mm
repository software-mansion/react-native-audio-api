#include <IOSOscillator.h>

namespace audiocontext {

	IOSOscillator::IOSOscillator() {
		OscillatorNode_ = [[OscillatorNode alloc] init];
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

	float IOSOscillator::getFrequency() const {
		return [OscillatorNode_ getFrequency];
	}

	void IOSOscillator::changeDetune(const float detune) const {
			[OscillatorNode_ changeDetune:detune];
	}

	float IOSOscillator::getDetune() const {
			return [OscillatorNode_ getDetune];
	}

	void IOSOscillator::setType(const std::string &type) const {
		NSString *nsType = [NSString stringWithUTF8String:type.c_str()];
		[OscillatorNode_ setType:nsType];
	}

	std::string IOSOscillator::getType() const {
		NSString *nsType = [OscillatorNode_ getType];
		return std::string([nsType UTF8String]);
	}

} // namespace audiocontext
