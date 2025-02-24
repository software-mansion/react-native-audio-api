#include "AudioAPIModule.h"

#include "AudioContext.h"
#include "AudioContextHostObject.h"

namespace audioapi {

using namespace facebook::jni;

AudioAPIModule::AudioAPIModule(
    jni::alias_ref<AudioAPIModule::jhybridobject> &jThis,
    jsi::Runtime *jsiRuntime,
    const std::shared_ptr<facebook::react::CallInvoker> &jsCallInvoker)
    : javaPart_(make_global(jThis)),
      jsiRuntime_(jsiRuntime),
      jsCallInvoker_(jsCallInvoker) {
  promiseVendor_ = std::make_shared<PromiseVendor>(jsiRuntime, jsCallInvoker);
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
  });
}

void AudioAPIModule::injectJSIBindings() {
  auto createAudioContext = jsi::Function::createFromHostFunction(
      *jsiRuntime_,
      jsi::PropNameID::forAscii(*jsiRuntime_, "createAudioContext"),
      0,
      [this](
          jsi::Runtime &runtime,
          const jsi::Value &thisValue,
          const jsi::Value *args,
          size_t count) -> jsi::Value {
        std::shared_ptr<AudioContext> audioContext;
        if (args[0].isUndefined()) {
          audioContext = std::make_shared<AudioContext>();
        } else {
          auto sampleRate = static_cast<float>(args[0].getNumber());
          audioContext = std::make_shared<AudioContext>(sampleRate);
        }

        auto audioContextHostObject = std::make_shared<AudioContextHostObject>(
            audioContext, promiseVendor_);

        return jsi::Object::createFromHostObject(
            runtime, audioContextHostObject);
      });

  jsiRuntime_->global().setProperty(
      *jsiRuntime_, "createAudioContext", createAudioContext);
}
} // namespace audioapi
