#pragma once

#include "AudioNodeWrapper.h"

#ifdef ANDROID
#include "AudioDestinationNode.h"
#include "AudioNodeWrapper.h"
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
    public:
        explicit AudioDestinationNodeWrapper() {}
#endif
    };
} // namespace audiocontext
