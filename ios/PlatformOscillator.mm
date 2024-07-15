#include <PlatformOscillator.h>
#include <iostream>

namespace audiocontext {

PlatformOscillator::PlatformOscillator() {
	iosOscillator_ = [[IOSOscillator alloc] init];
}

void PlatformOscillator::start() const {
	[iosOscillator_ start];
}

void PlatformOscillator::stop() const {
	[iosOscillator_ stop];
}

} // namespace audiocontext