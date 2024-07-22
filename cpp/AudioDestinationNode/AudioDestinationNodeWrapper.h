#pragma once

#include <memory>

#ifdef ANDROID
#include "AudioDestinationNode.h"
#include "OscillatorNodeWrapper.h"
#endif

namespace audiocontext {

#ifdef ANDROID
    class AudioDestinationNode;
#endif

    class AudioDestinationNodeWrapper {
#ifdef ANDROID
    private:
        friend class OscillatorNodeWrapper;

        std::shared_ptr<AudioDestinationNode> destination_;
#endif

    public:
#ifdef ANDROID
        explicit AudioDestinationNodeWrapper(std::shared_ptr<AudioDestinationNode> destination) : destination_(destination) {}
#else
        explicit AudioDestinationNodeWrapper() {}
#endif
    };
} // namespace audiocontext
