#include <audioapi/android/AudioAPIModule.h>

namespace audioapi {

using namespace facebook::jni;

AudioAPIModule::AudioAPIModule(
    jni::alias_ref<AudioAPIModule::jhybridobject> &jThis,
    jsi::Runtime *jsiRuntime,
    const std::shared_ptr<facebook::react::CallInvoker> &jsCallInvoker)
    : javaPart_(make_global(jThis)),
      jsiRuntime_(jsiRuntime),
      jsCallInvoker_(jsCallInvoker) {
  audioEventHandlerRegistry_ =
      std::make_shared<AudioEventHandlerRegistry>(jsiRuntime, jsCallInvoker);
}

jni::local_ref<AudioAPIModule::jhybriddata> AudioAPIModule::initHybrid(
    jni::alias_ref<jhybridobject> jThis,
    jlong jsContext,
    jni::alias_ref<facebook::react::CallInvokerHolder::javaobject>
        jsCallInvokerHolder) {
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
  });
}

void AudioAPIModule::injectJSIBindings() {
  AudioAPIModuleInstaller::injectJSIBindings(
      jsiRuntime_, jsCallInvoker_, audioEventHandlerRegistry_);
}

void AudioAPIModule::invokeHandlerWithEventNameAndEventBody(
    jni::alias_ref<jni::JString> eventName,
    jni::alias_ref<jni::JMap<jstring, jobject>> eventBody) {
  std::unordered_map<std::string, EventValue> body = {};

  for (const auto &entry : *eventBody) {
    std::string name = entry.first->toStdString();
    auto value = entry.second;

    if (value->isInstanceOf(jni::JString::javaClassStatic())) {
      body[name] = jni::static_ref_cast<jni::JString>(value)->toStdString();
    } else if (value->isInstanceOf(jni::JInteger::javaClassStatic())) {
      body[name] = jni::static_ref_cast<jni::JInteger>(value)->value();
    } else if (value->isInstanceOf(jni::JDouble::javaClassStatic())) {
      body[name] = jni::static_ref_cast<jni::JDouble>(value)->value();
    } else if (value->isInstanceOf(jni::JFloat::javaClassStatic())) {
      body[name] = jni::static_ref_cast<jni::JFloat>(value)->value();
    } else if (value->isInstanceOf(jni::JBoolean::javaClassStatic())) {
      auto booleanValue = jni::static_ref_cast<jni::JBoolean>(value)->value();

      if (booleanValue) {
        body[name] = true;
      } else {
        body[name] = false;
      }
    }
  }

  if (audioEventHandlerRegistry_ != nullptr) {
    audioEventHandlerRegistry_->invokeHandlerWithEventBody(
        eventName->toStdString(), body);
  }
}
} // namespace audioapi
