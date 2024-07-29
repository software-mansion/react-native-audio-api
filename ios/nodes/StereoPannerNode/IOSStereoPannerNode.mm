
#include <IOSStereoPannerNode.h>

namespace audiocontext {

    IOSStereoPannerNode::IOSStereoPannerNode(std::shared_ptr<IOSAudioContext> context) {
        audioNode_ = panner_ = [[StereoPannerNode alloc] init:context->audioContext_];
    }

    void IOSStereoPannerNode::setPan(const float pan) const {
        [panner_ setPan:pan];
    }

    float IOSStereoPannerNode::getPan() const {
        return [panner_ getPan];
    }
} // namespace audiocontext
