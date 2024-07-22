#pragma once

#include <fbjni/fbjni.h>
#include <react/jni/CxxModuleWrapper.h>
#include <react/jni/JMessageQueueThread.h>
#include <memory>
#include "AudioDestinationNode.h"

namespace audiocontext {

    using namespace facebook;
    using namespace facebook::jni;

    class AudioDestinationNode;

    class OscillatorNode : public jni::HybridClass<OscillatorNode> {
    public:
        static auto constexpr kJavaDescriptor = "Lcom/audiocontext/nodes/oscillator/OscillatorNode;";

        static jni::local_ref<OscillatorNode::jhybriddata> initHybrid(jni::alias_ref<jhybridobject> jThis)
        {
          return makeCxxInstance(jThis);
        }

        static void registerNatives() {
            registerHybrid({
                makeNativeMethod("initHybrid", OscillatorNode::initHybrid),
            });
        }

        void start(double time);
        void stop(double time);
        double getFrequency();
        double getDetune();
        std::string getWaveType();
        void setFrequency(double frequency);
        void setDetune(double detune);
        void setWaveType(const std::string& waveType);
        void connect(const AudioDestinationNode &destination);

    private:
        friend HybridBase;

        global_ref<OscillatorNode::javaobject> javaObject_;

        explicit OscillatorNode(jni::alias_ref<OscillatorNode::jhybridobject>& jThis);
    };

} // namespace audiocontext
