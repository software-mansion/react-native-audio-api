#pragma once

#include <fbjni/fbjni.h>
#include <react/jni/CxxModuleWrapper.h>
#include <react/jni/JMessageQueueThread.h>
#include <memory>
#include "AudioNode.h"

namespace audiocontext {

    using namespace facebook;
    using namespace facebook::jni;

    class StereoPannerNode : public jni::HybridClass<StereoPannerNode, AudioNode> {
    public:
        static auto constexpr kJavaDescriptor = "Lcom/audiocontext/nodes/StereoPannerNode;";

        double getPan();

        void setPan(double pan);
    };

} // namespace audiocontext
