#include <IOSOscillator.h>

namespace audiocontext {

    IOSOscillator::IOSOscillator(std::shared_ptr<IOSAudioContext> context) {
        audioNode_ = oscillatorNode_ = [[OscillatorNode alloc] initWithContext:context->audioContext_];
    }

    void IOSOscillator::start(double time) const {
        [oscillatorNode_ start:time];
    }

    void IOSOscillator::stop(double time) const {
        [oscillatorNode_ stop:time];
    }

    void IOSOscillator::setType(const std::string &type) const {
        NSString *nsType = [NSString stringWithUTF8String:type.c_str()];
        [oscillatorNode_ setType:nsType];
    }

    std::string IOSOscillator::getType() const {
        NSString *nsType = [oscillatorNode_ getType];
        return std::string([nsType UTF8String]);
    }

    std::shared_ptr<IOSAudioParam> IOSOscillator::getFrequencyParam() {
        std::shared_ptr<IOSAudioParam> param = std::make_shared<IOSAudioParam>();
        param->audioParam = oscillatorNode_.frequencyParam;
        return param;
    }

    std::shared_ptr<IOSAudioParam> IOSOscillator::getDetuneParam() {
        std::shared_ptr<IOSAudioParam> param = std::make_shared<IOSAudioParam>();
        param->audioParam = oscillatorNode_.detuneParam;
        return param;
    }
} // namespace audiocontext
