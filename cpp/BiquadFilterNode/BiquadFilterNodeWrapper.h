#pragma once

#include "AudioNodeWrapper.h"
#include "AudioParamWrapper.h"

#ifdef ANDROID
#include "BiquadFilterNode.h"
#else
#endif

namespace audiocontext {

#ifdef ANDROID
    class BiquadFilterNode;
#endif

    class BiquadFilterNodeWrapper: public AudioNodeWrapper {
#ifdef ANDROID
    private:
        BiquadFilterNode* getBiquadFilterNodeFromAudioNode();
    public:
        explicit BiquadFilterNodeWrapper(BiquadFilterNode *biquadFilterNode);
#else
    public:
        explicit BiquadFilterNodeWrapper(){};
#endif
    private:
        std::shared_ptr<AudioParamWrapper> frequencyParam_;
        std::shared_ptr<AudioParamWrapper> detuneParam_;
        std::shared_ptr<AudioParamWrapper> QParam_;
        std::shared_ptr<AudioParamWrapper> gainParam_;
    public:
        std::shared_ptr<AudioParamWrapper> getFrequencyParam() const;
        std::shared_ptr<AudioParamWrapper> getDetuneParam() const;
        std::shared_ptr<AudioParamWrapper> getQParam() const;
        std::shared_ptr<AudioParamWrapper> getGainParam() const;
        std::string getType();
        void setType(const std::string& filterType);
    };
} // namespace audiocontext
