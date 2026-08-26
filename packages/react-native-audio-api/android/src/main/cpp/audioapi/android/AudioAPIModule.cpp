#include <audioapi/android/AudioAPIModule.h>
#include <audioapi/android/JniEventPayloadParser.h>
#include <audioapi/android/core/AudioInputSelection.h>
#include <audioapi/android/system/NativeFileInfo.hpp>
#include <memory>

namespace audioapi {

using namespace facebook::jni;

AudioAPIModule::AudioAPIModule(
    jni::alias_ref<AudioAPIModule::jhybridobject> &jThis,
    jsi::Runtime *jsiRuntime,
    const std::shared_ptr<facebook::react::CallInvoker> &jsCallInvoker)
    : javaPart_(make_global(jThis)), jsiRuntime_(jsiRuntime), jsCallInvoker_(jsCallInvoker) {
  audioEventHandlerRegistry_ =
      std::make_shared<AudioEventHandlerRegistry>(jsiRuntime, jsCallInvoker);
}

jni::local_ref<AudioAPIModule::jhybriddata> AudioAPIModule::initHybrid(
    jni::alias_ref<jhybridobject> jThis,
    jlong jsContext,
    jni::alias_ref<facebook::react::CallInvokerHolder::javaobject> jsCallInvokerHolder) {
  auto jsCallInvoker = jsCallInvokerHolder->cthis()->getCallInvoker();
  auto rnRuntime = reinterpret_cast<jsi::Runtime *>(jsContext);
  return makeCxxInstance(jThis, rnRuntime, jsCallInvoker);
}

void AudioAPIModule::registerNatives() {
  registerHybrid({
      makeNativeMethod("initHybrid", AudioAPIModule::initHybrid),
      makeNativeMethod("injectJSIBindings", AudioAPIModule::injectJSIBindings),
      makeNativeMethod(
          "invokeHandlerWithEventNameAndEventBody",
          AudioAPIModule::invokeHandlerWithEventNameAndEventBody),
      makeNativeMethod("setPreferredInputDeviceId", AudioAPIModule::setPreferredInputDeviceId),
  });
}

void AudioAPIModule::injectJSIBindings() {
  // cache app directory paths on the attached thread
  NativeFileInfo::warmCache();

  AudioAPIModuleInstaller::injectJSIBindings(
      jsiRuntime_, jsCallInvoker_, audioEventHandlerRegistry_);
}

void AudioAPIModule::invokeHandlerWithEventNameAndEventBody(
    jint eventOrdinal,
    jni::alias_ref<jni::JMap<jstring, jobject>> eventBody) {
  if (audioEventHandlerRegistry_ == nullptr) {
    return;
  }

  auto event = static_cast<AudioEvent>(eventOrdinal);
  audioEventHandlerRegistry_->dispatchEvent(
      event, kBroadcastListenerId, buildPayloadFromJniMap(event, eventBody));
}

jboolean AudioAPIModule::setPreferredInputDeviceId(jint deviceId) {
  return static_cast<jboolean>(
      AudioInputSelection::setPreferredDeviceId(static_cast<int32_t>(deviceId)));
}

} // namespace audioapi
