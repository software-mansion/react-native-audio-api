#pragma once

#include <memory>

#ifdef ANDROID
#include "AudioDestinationNode.h"
#include "AudioNodeWrapper.h"
#endif

#include "AudioNodeWrapper.h"

namespace audiocontext {

#ifdef ANDROID
    class AudioDestinationNode;
#endif

    class AudioDestinationNodeWrapper: public AudioNodeWrapper {
#ifdef ANDROID
        private:
            std::shared_ptr<AudioDestinationNode> destinationNode_;

        public:
            explicit AudioDestinationNodeWrapper(const std::shared_ptr<AudioDestinationNode> &destinationNode) : AudioNodeWrapper(destinationNode), destinationNode_(destinationNode) {}
#else
        public:
            explicit AudioDestinationNodeWrapper() {}
#endif
    };
} // namespace audiocontext
