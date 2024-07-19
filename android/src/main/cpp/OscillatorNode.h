#pragma once

#include <fbjni/fbjni.h>
#include <jsi/jsi.h>
#include <react/jni/CxxModuleWrapper.h>
#include <react/jni/JMessageQueueThread.h>
#include <memory>
#include "OscillatorNodeHostObject.h"
#include "AudioDestinationNode.h"

namespace audiocontext {

    using namespace facebook;
    using namespace facebook::jni;

    class AudioDestinationNode;
    class OscillatorNodeHostObject;

    class OscillatorNode : public jni::HybridClass<OscillatorNode> {
    public:
        static auto constexpr kJavaDescriptor = "Lcom/audiocontext/nodes/oscillator/OscillatorNode;";

        static jni::local_ref<OscillatorNode::jhybriddata> initHybrid(jni::alias_ref<jhybridobject> jThis, jlong jsContext)
        {
          return makeCxxInstance(jThis, jsContext);
        }

        static void registerNatives() {
            registerHybrid({
                makeNativeMethod("initHybrid", OscillatorNode::initHybrid),
            });
        }

        void start();
        void stop();
        double getFrequency();
        double getDetune();
        std::string getWaveType();
        void setFrequency(double frequency);
        void setDetune(double detune);
        void setWaveType(const std::string& waveType);
        void connect(const AudioDestinationNode &destination);

        std::shared_ptr<OscillatorNodeHostObject> createOscillatorNodeHostObject();

    private:
        friend HybridBase;

        global_ref<OscillatorNode::javaobject> javaObject_;
        jlong jsContext_;

        explicit OscillatorNode(jni::alias_ref<OscillatorNode::jhybridobject>& jThis, jlong jsContext);
    };

} // namespace audiocontext
