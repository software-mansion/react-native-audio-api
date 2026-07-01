#include <audioapi/android/AudioAPIModule.h>
#include <audioapi/android/JniEventPayloadParser.h>
#include <audioapi/android/system/NativeFileInfo.hpp>
#include <memory>

namespace audioapi {

using namespace facebook::jni;

AudioAPIModule::AudioAPIModule(
    jni::alias_ref<AudioAPIModule::jhybridobject> &jThis,
#if RN_AUDIO_API_ENABLE_WORKLETS
    std::weak_ptr<WorkletsModuleProxy> weakWorkletsModuleProxy,
#endif
    jsi::Runtime *jsiRuntime,
    const std::shared_ptr<facebook::react::CallInvoker> &jsCallInvoker)
    : javaPart_(make_global(jThis)),
      jsiRuntime_(jsiRuntime),
#if RN_AUDIO_API_ENABLE_WORKLETS
      weakWorkletsModuleProxy_(weakWorkletsModuleProxy),
#endif
      jsCallInvoker_(jsCallInvoker) {
  audioEventHandlerRegistry_ =
      std::make_shared<AudioEventHandlerRegistry>(jsiRuntime, jsCallInvoker);
}

jni::local_ref<AudioAPIModule::jhybriddata> AudioAPIModule::initHybrid(
    jni::alias_ref<jhybridobject> jThis,
    jni::alias_ref<jni::JObject> jWorkletsModule,
    jlong jsContext,
    jni::alias_ref<facebook::react::CallInvokerHolder::javaobject> jsCallInvokerHolder) {
  auto jsCallInvoker = jsCallInvokerHolder->cthis()->getCallInvoker();
  auto rnRuntime = reinterpret_cast<jsi::Runtime *>(jsContext);
#if RN_AUDIO_API_ENABLE_WORKLETS
  if (jWorkletsModule) {
    auto castedModule = jni::static_ref_cast<WorkletsModule::javaobject>(jWorkletsModule);
    auto workletsModuleProxy = castedModule->cthis()->getWorkletsModuleProxy();
    return makeCxxInstance(jThis, workletsModuleProxy, rnRuntime, jsCallInvoker);
  }
  throw std::runtime_error("Worklets module is required but not provided from Java/Kotlin side");
#else
  return makeCxxInstance(jThis, rnRuntime, jsCallInvoker);
#endif
}

void AudioAPIModule::registerNatives() {
  registerHybrid({
      makeNativeMethod("initHybrid", AudioAPIModule::initHybrid),
      makeNativeMethod("injectJSIBindings", AudioAPIModule::injectJSIBindings),
      makeNativeMethod(
          "invokeHandlerWithEventNameAndEventBody",
          AudioAPIModule::invokeHandlerWithEventNameAndEventBody),
  });
}

void AudioAPIModule::injectJSIBindings() {
  // cache app directory paths on the attached thread
  NativeFileInfo::warmCache();

#if RN_AUDIO_API_ENABLE_WORKLETS
  auto uiWorkletRuntime = weakWorkletsModuleProxy_.lock()->getUIWorkletRuntime();
#else
  auto uiWorkletRuntime = nullptr;
#endif
  AudioAPIModuleInstaller::injectJSIBindings(
      jsiRuntime_, jsCallInvoker_, audioEventHandlerRegistry_, uiWorkletRuntime);
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

} // namespace audioapi
