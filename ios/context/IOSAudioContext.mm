#include <IOSAudioContext.h>

namespace audiocontext {

	IOSAudioContext::IOSAudioContext() {
        audioContext_ = [[AudioContext alloc] init];
	}

} // namespace audiocontext
