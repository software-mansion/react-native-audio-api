#pragma once

#include <fbjni/fbjni.h>
#include <react/jni/CxxModuleWrapper.h>
#include <react/jni/JMessageQueueThread.h>
#include <memory>
#include "AudioNode.h"

namespace audiocontext {

    using namespace facebook;
    using namespace facebook::jni;

    class AudioDestinationNode : public jni::HybridClass<AudioDestinationNode, AudioNode> {
    public:
        static auto constexpr kJavaDescriptor = "Lcom/audiocontext/nodes/AudioDestinationNode;";
    };

} // namespace audiocontext
