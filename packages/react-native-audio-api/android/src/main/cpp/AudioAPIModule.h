#pragma once

#include <ReactCommon/CallInvokerHolder.h>
#include <fbjni/fbjni.h>
#include <react/jni/CxxModuleWrapper.h>
#include <react/jni/JMessageQueueThread.h>
#include <memory>
#include <utility>
#include "AudioAPIModuleInstaller.h"

namespace audioapi {

using namespace facebook;
using namespace react;

class AudioAPIModule : public jni::HybridClass<AudioAPIModule> {
 public:
  static auto constexpr kJavaDescriptor =
      "Lcom/swmansion/audioapi/AudioAPIModule;";

  static jni::local_ref<AudioAPIModule::jhybriddata> initHybrid(
      jni::alias_ref<jhybridobject> jThis,
      jlong jsContext,
      jni::alias_ref<facebook::react::CallInvokerHolder::javaobject>
          jsCallInvokerHolder);

  static void registerNatives();

  void injectJSIBindings();

 private:
  friend HybridBase;

  jni::global_ref<AudioAPIModule::javaobject> javaPart_;
  jsi::Runtime *jsiRuntime_;
  std::shared_ptr<facebook::react::CallInvoker> jsCallInvoker_;

  explicit AudioAPIModule(
      jni::alias_ref<AudioAPIModule::jhybridobject> &jThis,
      jsi::Runtime *jsiRuntime,
      const std::shared_ptr<facebook::react::CallInvoker> &jsCallInvoker);
};

} // namespace audioapi
