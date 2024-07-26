#pragma once

#include <memory>

#ifdef ANDROID
#include "AudioNode.h"
#else
#include "IOSAudioNode.h"
#endif

namespace audiocontext {
    class AudioNodeWrapper {
    public:
#ifdef ANDROID
        std::shared_ptr<AudioNode> node_;

        explicit AudioNodeWrapper(std::shared_ptr<AudioNode> node) : node_(node) {}
#else
        std::shared_ptr<IOSAudioNode> node_;
        explicit AudioNodeWrapper() {}
#endif
        void connect(const std::shared_ptr<AudioNodeWrapper> node) const;
        void disconnect(const std::shared_ptr<AudioNodeWrapper> node) const;
    };
} // namespace audiocontext
