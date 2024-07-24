#pragma once

#include <memory>

#ifdef ANDROID
#include "AudioNode.h"
#endif

namespace audiocontext {
    using namespace facebook;

    class AudioNodeWrapper {
#ifdef ANDROID
    protected:
        std::shared_ptr<AudioNode> node_;
    public:
        explicit AudioNodeWrapper(const std::shared_ptr<AudioNode> &node) : node_(node) {}
#else
    public:
        explicit AudioNodeWrapper() {}
#endif
    public:
        void connect(const std::shared_ptr<AudioNodeWrapper> &node) const;
        void disconnect(const std::shared_ptr<AudioNodeWrapper> &node) const;
    };
} // namespace audiocontext
