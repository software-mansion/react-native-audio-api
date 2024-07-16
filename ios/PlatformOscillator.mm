#include <PlatformOscillator.h>
#include <iostream>

namespace audiocontext {

PlatformOscillator::PlatformOscillator(const float frequency) : frequency_(frequency) {
	iosOscillator_ = [[IOSOscillator alloc] init:frequency];
}

void PlatformOscillator::start() const {
	[iosOscillator_ start];
}

void PlatformOscillator::stop() const {
	[iosOscillator_ stop];
}

void PlatformOscillator::changeFrequency(const float frequency) const {
    [iosOscillator_ changeFrequency:frequency];
}

} // namespace audiocontext
