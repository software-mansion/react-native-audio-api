#pragma once

#include <fbjni/fbjni.h>
#include <react/jni/CxxModuleWrapper.h>
#include <react/jni/JMessageQueueThread.h>
#include <memory>
#include "AudioNode.h"
#include "AudioParam.h"

namespace audiocontext {

    using namespace facebook;
    using namespace facebook::jni;

    class BiquadFilterNode : public jni::HybridClass<BiquadFilterNode, AudioNode> {
    public:
        static auto constexpr kJavaDescriptor = "Lcom/audiocontext/nodes/filter/BiquadFilterNode;";

        std::shared_ptr<AudioParam> getFrequencyParam();
        std::shared_ptr<AudioParam> getDetuneParam();
        std::shared_ptr<AudioParam> getQParam();
        std::shared_ptr<AudioParam> getGainParam();
        std::string getFilterType();
        void setFilterType(const std::string &filterType);
    };

} // namespace audiocontext
