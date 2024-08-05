#pragma once

#include <fbjni/fbjni.h>
#include <react/jni/CxxModuleWrapper.h>
#include <react/jni/JMessageQueueThread.h>
#include <memory>

namespace audiocontext {

    using namespace facebook;
    using namespace facebook::jni;

    class AudioNode : public jni::HybridClass<AudioNode> {
    public:
        static auto constexpr kJavaDescriptor = "Lcom/audiocontext/nodes/AudioNode;";

        static jni::local_ref<AudioNode::jhybriddata> initHybrid(jni::alias_ref<jhybridobject> jThis)
        {
            return makeCxxInstance(jThis);
        }

        static void registerNatives() {
            registerHybrid({
                   makeNativeMethod("initHybrid", AudioNode::initHybrid),
           });
        }

        int getNumberOfInputs();
        int getNumberOfOutputs();
        void connect(const std::shared_ptr<AudioNode> &node);
        void disconnect(const std::shared_ptr<AudioNode> &node);
        virtual void prepareForDeconstruction();

    protected:
        friend HybridBase;

        global_ref<AudioNode::javaobject> javaPart_;

        explicit AudioNode(jni::alias_ref<AudioNode::jhybridobject>& jThis);
    };

} // namespace audiocontext
