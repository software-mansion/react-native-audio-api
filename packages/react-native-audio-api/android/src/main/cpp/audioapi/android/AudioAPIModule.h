#pragma once

#include <audioapi/AudioAPIModuleInstaller.h>
#include <audioapi/events/AudioEventHandlerRegistry.h>

#include <ReactCommon/CallInvokerHolder.h>
#include <fbjni/fbjni.h>
#include <react/jni/CxxModuleWrapper.h>
#include <react/jni/JMessageQueueThread.h>
#include <audioapi/core/utils/worklets/SafeIncludes.h>
#include <memory>
#include <utility>
#include <unordered_map>

namespace audioapi {

using namespace facebook;
using namespace react;
using namespace worklets;

class AudioAPIModule : public jni::HybridClass<AudioAPIModule> {
 public:
  static auto constexpr kJavaDescriptor =
      "Lcom/swmansion/audioapi/AudioAPIModule;";

  static jni::local_ref<AudioAPIModule::jhybriddata> initHybrid(
      jni::alias_ref<jhybridobject> jThis,
      jni::alias_ref<jni::JObject> jWorkletsModule, // it will be null if RN_AUDIO_API_ENABLE_WORKLETS is false
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
  #if RN_AUDIO_API_ENABLE_WORKLETS
  std::weak_ptr<worklets::WorkletsModuleProxy> weakWorkletsModuleProxy_;
  #endif
  std::shared_ptr<facebook::react::CallInvoker> jsCallInvoker_;
  std::shared_ptr<AudioEventHandlerRegistry> audioEventHandlerRegistry_;

  #if RN_AUDIO_API_ENABLE_WORKLETS
  explicit AudioAPIModule(
      jni::alias_ref<AudioAPIModule::jhybridobject> &jThis,
      std::weak_ptr<worklets::WorkletsModuleProxy> weakWorkletsModuleProxy,
      jsi::Runtime *jsiRuntime,
      const std::shared_ptr<facebook::react::CallInvoker> &jsCallInvoker);
  #else
  explicit AudioAPIModule(
      jni::alias_ref<AudioAPIModule::jhybridobject> &jThis,
      jsi::Runtime *jsiRuntime,
      const std::shared_ptr<facebook::react::CallInvoker> &jsCallInvoker);
  #endif
};

} // namespace audioapi
