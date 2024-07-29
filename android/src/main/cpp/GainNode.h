#pragma once

#include <fbjni/fbjni.h>
#include <react/jni/CxxModuleWrapper.h>
#include <react/jni/JMessageQueueThread.h>
#include <memory>
#include "AudioNode.h"

namespace audiocontext {

    using namespace facebook;
    using namespace facebook::jni;

    class GainNode : public jni::HybridClass<GainNode, AudioNode> {
    public:
        static auto constexpr kJavaDescriptor = "Lcom/audiocontext/nodes/GainNode;";

        double getGain();

        void setGain(double gain);
    };

} // namespace audiocontext
