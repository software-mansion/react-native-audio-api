#include <IOSGainNode.h>

namespace audiocontext {

    IOSGainNode::IOSGainNode(std::shared_ptr<IOSAudioContext> context) {
        audioNode_ = gainNode_ = [[GainNode alloc] initWithContext:context->audioContext_];
    }

    IOSGainNode::~IOSGainNode() {
        [gainNode_ clean];
        audioNode_ = gainNode_ = nil;
    }

    std::shared_ptr<IOSAudioParam> IOSGainNode::getAudioParam() {
        std::shared_ptr<IOSAudioParam> param = std::make_shared<IOSAudioParam>();
        param->audioParam = gainNode_.audioParam;
        return param;
    }
} // namespace audiocontext
