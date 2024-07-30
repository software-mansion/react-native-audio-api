#pragma once

#include <fbjni/fbjni.h>
#include <react/jni/CxxModuleWrapper.h>
#include <react/jni/JMessageQueueThread.h>
#include <memory>

namespace audiocontext {

    using namespace facebook;
    using namespace facebook::jni;

    class AudioParam : public jni::HybridClass<AudioParam> {
    public:
        static auto constexpr kJavaDescriptor = "Lcom/audiocontext/nodes/parameters/AudioParam;";

        static jni::local_ref<AudioParam::jhybriddata> initHybrid(jni::alias_ref<jhybridobject> jThis)
        {
            return makeCxxInstance(jThis);
        }

        static void registerNatives() {
            registerHybrid({
                   makeNativeMethod("initHybrid", AudioParam::initHybrid),
           });
        }

        double getValue();
        void setValue(double value);
        double getDefaultValue();
        double getMinValue();
        double getMaxValue();
        void setValueAtTime(double value, double startTime);
        void linearRampToValueAtTime(double value, double endTime);

    protected:
        friend HybridBase;

        global_ref<AudioParam::javaobject> javaObject_;

        explicit AudioParam(jni::alias_ref<AudioParam::jhybridobject>& jThis);
    };

} // namespace audiocontext
