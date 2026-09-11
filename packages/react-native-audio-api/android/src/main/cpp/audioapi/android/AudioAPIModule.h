#pragma once

#include <ReactCommon/CallInvokerHolder.h>
#include <audioapi/AudioAPIModuleInstaller.h>
#include <audioapi/events/AudioEventHandlerRegistry.h>
#include <fbjni/fbjni.h>
#include <memory>

namespace audioapi {

using namespace facebook;

class AudioAPIModule : public jni::HybridClass<AudioAPIModule> {
 public:
  static auto constexpr kJavaDescriptor = "Lcom/swmansion/audioapi/AudioAPIModule;";

  static jni::local_ref<AudioAPIModule::jhybriddata> initHybrid(
      jni::alias_ref<jhybridobject> jThis,
      jlong jsContext,
      jni::alias_ref<facebook::react::CallInvokerHolder::javaobject> jsCallInvokerHolder);

  static void registerNatives();

  void injectJSIBindings();
  void invokeHandlerWithEventNameAndEventBody(
      jint eventOrdinal,
      jni::alias_ref<jni::JMap<jstring, jobject>> eventBody);

  /// @brief Hands the capture device chosen through AudioManager.setInputDevice
  /// to the recorders.
  /// @returns false when a capture stream is already running, in which case the
  /// selection is left unchanged. See AudioInputSelection.
  jboolean setPreferredInputDeviceId(jint deviceId);

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
