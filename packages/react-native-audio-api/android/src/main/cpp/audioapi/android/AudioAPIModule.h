#pragma once

#include <audioapi/AudioAPIModuleInstaller.h>
#include <audioapi/events/AudioEventHandlerRegistry.h>

#include <ReactCommon/CallInvokerHolder.h>
#include <fbjni/fbjni.h>
#include <react/jni/CxxModuleWrapper.h>
#include <react/jni/JMessageQueueThread.h>
#include <memory>
#include <utility>
#include <unordered_map>

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
  void invokeHandlerWithEventNameAndEventBody(jni::alias_ref<jni::JString> eventName, jni::alias_ref<jni::JMap<jstring, jobject>> eventBody);

 private:
  friend HybridBase;

  jni::global_ref<AudioAPIModule::javaobject> javaPart_;
  jsi::Runtime *jsiRuntime_;
  std::shared_ptr<facebook::react::CallInvoker> jsCallInvoker_;
  std::shared_ptr<AudioEventHandlerRegistry> audioEventHandlerRegistry_;

  explicit AudioAPIModule(
      jni::alias_ref<AudioAPIModule::jhybridobject> &jThis,
      jsi::Runtime *jsiRuntime,
      const std::shared_ptr<facebook::react::CallInvoker> &jsCallInvoker);
};

} // namespace audioapi
