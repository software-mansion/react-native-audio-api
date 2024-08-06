#include <IOSAudioContext.h>

namespace audiocontext {

    IOSAudioContext::IOSAudioContext() {
        audioContext_ = [[AudioContext alloc] init];
    }

    IOSAudioContext::~IOSAudioContext() {
        [audioContext_ clean];
        audioContext_ = nil;
    }

    double IOSAudioContext::getCurrentTime() {
        return [audioContext_ getCurrentTime];
    }

    std::string IOSAudioContext::getState() {
        NSString *nsType = [audioContext_ getState];
        return std::string([nsType UTF8String]);
    }

    double IOSAudioContext::getSampleRate() {
        return [audioContext_ getSampleRate];
    }
} // namespace audiocontext
