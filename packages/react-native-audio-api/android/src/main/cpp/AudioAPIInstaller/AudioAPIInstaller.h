#pragma once

#include <fbjni/fbjni.h>
#include <jsi/jsi.h>
#include <react/jni/CxxModuleWrapper.h>
#include <react/jni/JMessageQueueThread.h>
#include <memory>
#include <utility>

#include "AudioAPIInstallerHostObject.h"
#include "AudioAPIInstallerWrapper.h"
#include "AudioContext.h"

namespace audioapi {

using namespace facebook;
using namespace facebook::jni;

class AudioAPIInstaller : public jni::HybridClass<AudioAPIInstaller> {
 public:
  static auto constexpr kJavaDescriptor =
      "Lcom/swmansion/audioapi/module/AudioAPIInstaller;";

  static jni::local_ref<AudioAPIInstaller::jhybriddata> initHybrid(
      jni::alias_ref<jhybridobject> jThis) {
    return makeCxxInstance(jThis);
  }

  static void registerNatives() {
    registerHybrid({
        makeNativeMethod("initHybrid", AudioAPIInstaller::initHybrid),
        makeNativeMethod("install", AudioAPIInstaller::install),
    });
  }

  std::shared_ptr<AudioContext> createAudioContext();
  void install(jlong jsContext);

 private:
  friend HybridBase;

  global_ref<AudioAPIInstaller::javaobject> javaPart_;

  explicit AudioAPIInstaller(
      jni::alias_ref<AudioAPIInstaller::jhybridobject> &jThis);
};

} // namespace audioapi
