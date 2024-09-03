#pragma once

#include "AudioNode.h"
#include "AudioParam.h"

namespace audiocontext {

    using namespace facebook;
    using namespace facebook::jni;

    class OscillatorNode : public jni::HybridClass<OscillatorNode, AudioNode> {
    public:
        static auto constexpr kJavaDescriptor = "Lcom/audiocontext/nodes/oscillator/OscillatorNode;";

        void start(double time);
        void stop(double time);
        AudioParam* getFrequencyParam();
        AudioParam* getDetuneParam();
        std::string getWaveType();
        void setWaveType(const std::string &waveType);
        void prepareForDeconstruction();
    };
}// namespace audiocontext
