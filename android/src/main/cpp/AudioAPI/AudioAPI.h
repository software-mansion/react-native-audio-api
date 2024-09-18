#pragma once

#include <fbjni/fbjni.h>
#include <jsi/jsi.h>
#include <react/jni/CxxModuleWrapper.h>
#include <react/jni/JMessageQueueThread.h>
#include <memory>
#include <utility>

#include "AudioAPIWrapper.h"
#include "AudioAPIHostObject.h"
#include "AudioContext.h"

namespace audioapi {

    using namespace facebook;
    using namespace facebook::jni;

    class AudioAPI : public jni::HybridClass<AudioAPI> {
    public:
        static auto constexpr kJavaDescriptor =
                "Lcom/swmansion/audioapi/module/AudioAPI;";

        static jni::local_ref<AudioAPI::jhybriddata> initHybrid(
                jni::alias_ref<jhybridobject> jThis) {
            return makeCxxInstance(jThis);
        }

        static void registerNatives() {
            registerHybrid({
                   makeNativeMethod("initHybrid", AudioAPI::initHybrid),
                   makeNativeMethod("install", AudioAPI::install),
           });
        }

        AudioContext *createAudioContext();
        void install(jlong jsContext);

    private:
        friend HybridBase;

        global_ref<AudioAPI::javaobject> javaPart_;

        explicit AudioAPI(jni::alias_ref<AudioAPI::jhybridobject> &jThis);
    };

} // namespace audioapi
