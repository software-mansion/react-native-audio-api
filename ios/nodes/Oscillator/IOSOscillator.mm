#include <IOSOscillator.h>

namespace audiocontext {

    IOSOscillator::IOSOscillator(std::shared_ptr<IOSAudioContext> context) {
        audioNode_ = oscillatorNode_ = [[OscillatorNode alloc] init:context->audioContext_];
    }

    void IOSOscillator::start() const {
        [oscillatorNode_ start];
    }

    void IOSOscillator::stop() const {
        [oscillatorNode_ stop];
    }

    void IOSOscillator::setFrequency(const float frequency) const {
        [oscillatorNode_ setFrequency:frequency];
    }

    float IOSOscillator::getFrequency() const {
        return [oscillatorNode_ getFrequency];
    }

    void IOSOscillator::setDetune(const float detune) const {
        [oscillatorNode_ setDetune:detune];
    }

    float IOSOscillator::getDetune() const {
        return [oscillatorNode_ getDetune];
    }

    void IOSOscillator::setType(const std::string &type) const {
        NSString *nsType = [NSString stringWithUTF8String:type.c_str()];
        [oscillatorNode_ setType:nsType];
    }

    std::string IOSOscillator::getType() const {
        NSString *nsType = [oscillatorNode_ getType];
        return std::string([nsType UTF8String]);
    }
} // namespace audiocontext
