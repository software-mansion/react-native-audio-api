#include <IOSOscillator.h>

namespace audiocontext {

	IOSOscillator::IOSOscillator(const float frequency) {
		OscillatorNode_ = [[OscillatorNode alloc] initWithFrequency:frequency];
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

	void IOSOscillator::setType(const std::string &type) const {
		NSString *nsType = [NSString stringWithUTF8String:type.c_str()];
		[OscillatorNode_ setType:nsType];
	}

    std::string IOSOscillator::getType() const {
		NSString *nsType = [OscillatorNode_ getType];
		return std::string([nsType UTF8String]);
	}

} // namespace audiocontext
