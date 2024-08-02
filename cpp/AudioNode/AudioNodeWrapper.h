#pragma once

#include <memory>

#ifdef ANDROID
#include "AudioNode.h"
#else
#include "IOSAudioNode.h"
#endif

namespace audiocontext {
    class AudioNodeWrapper {
#ifdef ANDROID
        protected:
            std::shared_ptr<AudioNode> node_;
        public:
            explicit AudioNodeWrapper(const std::shared_ptr<AudioNode> &node) : node_(node) {}
#else
        public:
            std::shared_ptr<IOSAudioNode> node_;
            explicit AudioNodeWrapper() {}
#endif
        public:
            int getNumberOfInputs() const;
            int getNumberOfOutputs() const;
            void connect(const std::shared_ptr<AudioNodeWrapper> &node) const;
            void disconnect(const std::shared_ptr<AudioNodeWrapper> &node) const;
    };
} // namespace audiocontext
