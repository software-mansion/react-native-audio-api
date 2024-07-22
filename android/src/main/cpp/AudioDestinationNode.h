#pragma once

#include <fbjni/fbjni.h>
#include <react/jni/CxxModuleWrapper.h>
#include <react/jni/JMessageQueueThread.h>
#include <memory>
#include "OscillatorNode.h"

namespace audiocontext {

    using namespace facebook;
    using namespace facebook::jni;

    class OscillatorNode;

    class AudioDestinationNode : public jni::HybridClass<AudioDestinationNode> {
    public:
        static auto constexpr kJavaDescriptor = "Lcom/audiocontext/nodes/AudioDestinationNode;";

        static jni::local_ref<AudioDestinationNode::jhybriddata> initHybrid(jni::alias_ref<jhybridobject> jThis)
        {
            return makeCxxInstance(jThis);
        }

        static void registerNatives() {
            registerHybrid({
               makeNativeMethod("initHybrid", AudioDestinationNode::initHybrid),
           });
        }

    private:
        friend HybridBase;
        friend class OscillatorNode;

        global_ref<AudioDestinationNode::javaobject> javaObject_;

        explicit AudioDestinationNode(jni::alias_ref<AudioDestinationNode::jhybridobject>& jThis);
    };

} // namespace audiocontext
