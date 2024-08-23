
#include <IOSStereoPannerNode.h>

namespace audiocontext {

    IOSStereoPannerNode::IOSStereoPannerNode(std::shared_ptr<IOSAudioContext> context) {
        audioNode_ = panner_ = [[StereoPannerNode alloc] initWithContext:context->audioContext_];
    }

    IOSStereoPannerNode::~IOSStereoPannerNode() {
        [panner_ cleanup];
        audioNode_ = panner_ = nil;
    }

    std::shared_ptr<IOSAudioParam> IOSStereoPannerNode::getPanParam() {
        std::shared_ptr<IOSAudioParam> param = std::make_shared<IOSAudioParam>();
        param->audioParam = panner_.panParam;
        return param;
    }
} // namespace audiocontext
