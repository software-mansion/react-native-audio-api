#pragma once

#include "AudioNodeWrapper.h"
#include "AudioParamWrapper.h"

#ifdef ANDROID
#include "OscillatorNode.h"
#else
#include "IOSOscillator.h"
#endif

namespace audiocontext {

#ifdef ANDROID
    class OscillatorNode;
#endif

    class OscillatorNodeWrapper: public AudioNodeWrapper {
#ifdef ANDROID
    public:
        explicit OscillatorNodeWrapper(const std::shared_ptr<OscillatorNode> &oscillator);
        ~OscillatorNodeWrapper();
#else
        private:
            std::shared_ptr<IOSOscillator> oscillator_;
        public:
            explicit OscillatorNodeWrapper(std::shared_ptr<IOSAudioContext> context);
#endif
        private:
            std::shared_ptr<AudioParamWrapper> frequencyParam_;
            std::shared_ptr<AudioParamWrapper> detuneParam_;
        public:
            std::shared_ptr<AudioParamWrapper> getFrequencyParam();
            std::shared_ptr<AudioParamWrapper> getDetuneParam();
            std::string getType();
            void start(double time);
            void stop(double time);
            void setType(const std::string& type);
    };
} // namespace audiocontext
