#include <IOSBiquadFilterNode.h>

namespace audiocontext {

    IOSBiquadFilterNode::IOSBiquadFilterNode(std::shared_ptr<IOSAudioContext> context) {
        audioNode_ = biquadFilterNode_ = [[BiquadFilterNode alloc] initWithContext:context->audioContext_];
    }

    IOSBiquadFilterNode::~IOSBiquadFilterNode() {
        [biquadFilterNode_ cleanup];
        audioNode_ = biquadFilterNode_ = nil;
    }

    void IOSBiquadFilterNode::setType(const std::string &type) const {
        NSString *nsType = [NSString stringWithUTF8String:type.c_str()];
        [biquadFilterNode_ setType:nsType];
    }

    std::string IOSBiquadFilterNode::getType() const {
        NSString *nsType = [biquadFilterNode_ getType];
        return std::string([nsType UTF8String]);
    }

    std::shared_ptr<IOSAudioParam> IOSBiquadFilterNode::getFrequencyParam() {
        std::shared_ptr<IOSAudioParam> param = std::make_shared<IOSAudioParam>();
        param->audioParam = biquadFilterNode_.frequencyParam;
        return param;
    }

    std::shared_ptr<IOSAudioParam> IOSBiquadFilterNode::getDetuneParam() {
        std::shared_ptr<IOSAudioParam> param = std::make_shared<IOSAudioParam>();
        param->audioParam = biquadFilterNode_.detuneParam;
        return param;
    }

    std::shared_ptr<IOSAudioParam> IOSBiquadFilterNode::getQParam() {
        std::shared_ptr<IOSAudioParam> param = std::make_shared<IOSAudioParam>();
        param->audioParam = biquadFilterNode_.QParam;
        return param;
    }

    std::shared_ptr<IOSAudioParam> IOSBiquadFilterNode::getGainParam() {
        std::shared_ptr<IOSAudioParam> param = std::make_shared<IOSAudioParam>();
        param->audioParam = biquadFilterNode_.gainParam;
        return param;
    }
} // namespace audiocontext

