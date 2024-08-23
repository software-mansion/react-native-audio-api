#include <IOSOscillatorNode.h>

namespace audiocontext {

    IOSOscillatorNode::IOSOscillatorNode(std::shared_ptr<IOSAudioContext> context) {
        audioNode_ = oscillatorNode_ = [[OscillatorNode alloc] initWithContext:context->audioContext_];
    }

    IOSOscillatorNode::~IOSOscillatorNode() {
        [oscillatorNode_ cleanup];
        audioNode_ = oscillatorNode_ = nil;
    }

    void IOSOscillatorNode::start(double time) const {
        [oscillatorNode_ start:time];
    }

    void IOSOscillatorNode::stop(double time) const {
        [oscillatorNode_ stop:time];
    }

    void IOSOscillatorNode::setType(const std::string &type) const {
        NSString *nsType = [NSString stringWithUTF8String:type.c_str()];
        [oscillatorNode_ setType:nsType];
    }

    std::string IOSOscillatorNode::getType() const {
        NSString *nsType = [oscillatorNode_ getType];
        return std::string([nsType UTF8String]);
    }

    std::shared_ptr<IOSAudioParam> IOSOscillatorNode::getFrequencyParam() {
        std::shared_ptr<IOSAudioParam> param = std::make_shared<IOSAudioParam>();
        param->audioParam = oscillatorNode_.frequencyParam;
        return param;
    }

    std::shared_ptr<IOSAudioParam> IOSOscillatorNode::getDetuneParam() {
        std::shared_ptr<IOSAudioParam> param = std::make_shared<IOSAudioParam>();
        param->audioParam = oscillatorNode_.detuneParam;
        return param;
    }
} // namespace audiocontext
