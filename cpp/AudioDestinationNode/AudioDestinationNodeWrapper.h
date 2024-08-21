#pragma once

#include "AudioNodeWrapper.h"

#ifdef ANDROID
#include "AudioDestinationNode.h"
#include "AudioNodeWrapper.h"
#else
#include "IOSAudioDestinationNode.h"
#include "IOSAudioContext.h"
#endif

namespace audiocontext {

#ifdef ANDROID
    class AudioDestinationNode;
#endif

    class AudioDestinationNodeWrapper: public AudioNodeWrapper {
#ifdef ANDROID
    public:
        explicit AudioDestinationNodeWrapper(const std::shared_ptr<AudioDestinationNode> &destinationNode) : AudioNodeWrapper(destinationNode) {}
#else
    private:
        std::shared_ptr<IOSAudioDestinationNode> destination_;
    public:
        explicit AudioDestinationNodeWrapper(std::shared_ptr<IOSAudioContext> context) : AudioNodeWrapper() {
            node_ = destination_ = std::make_shared<IOSAudioDestinationNode>(context);
        }
#endif
    };
} // namespace audiocontext
