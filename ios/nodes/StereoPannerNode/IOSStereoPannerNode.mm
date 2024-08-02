
#include <IOSStereoPannerNode.h>

namespace audiocontext {

    IOSStereoPannerNode::IOSStereoPannerNode(std::shared_ptr<IOSAudioContext> context) {
        audioNode_ = panner_ = [[StereoPannerNode alloc] init:context->audioContext_];
    }

    std::shared_ptr<IOSAudioParam> IOSStereoPannerNode::getAudioParam() {
        std::shared_ptr<IOSAudioParam> param = std::make_shared<IOSAudioParam>();
        param->audioParam = panner_.audioParam;
        return param;
    }
} // namespace audiocontext
