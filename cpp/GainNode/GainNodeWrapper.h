#pragma once

#include "AudioNodeWrapper.h"
#include "AudioParamWrapper.h"

#ifdef ANDROID
#include "GainNode.h"
#endif

namespace audiocontext {

#ifdef ANDROID
    class GainNode;
#endif

    class GainNodeWrapper: public AudioNodeWrapper {
#ifdef ANDROID
    public:
        explicit GainNodeWrapper(const std::shared_ptr<GainNode> &gainNode);
#else
    public:
        explicit GainNodeWrapper() {}
#endif
    private:
        std::shared_ptr<AudioParamWrapper> gainParam_;
    public:
        std::shared_ptr<AudioParamWrapper> getGainParam();
    };
} // namespace audiocontext
