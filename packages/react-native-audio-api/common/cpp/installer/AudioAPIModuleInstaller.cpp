#include "AudioAPIModuleInstaller.h"

#include "AudioContext.h"
#include "AudioContextHostObject.h"

namespace audioapi {

AudioAPIModuleInstaller::AudioAPIModuleInstaller(
    jsi::Runtime *jsiRuntime,
    const std::shared_ptr<facebook::react::CallInvoker> &jsCallInvoker) {
  promiseVendor_ = std::make_shared<PromiseVendor>(jsiRuntime, jsCallInvoker);
}

void AudioAPIModuleInstaller::injectJSIBindings(jsi::Runtime *jsiRuntime) {
  auto createAudioContext = getCreateAudioContextFunction(jsiRuntime);
  jsiRuntime->global().setProperty(
      *jsiRuntime, "createAudioContext", createAudioContext);
}

jsi::Function AudioAPIModuleInstaller::getCreateAudioContextFunction(
    jsi::Runtime *jsiRuntime) {
  return jsi::Function::createFromHostFunction(
      *jsiRuntime,
      jsi::PropNameID::forAscii(*jsiRuntime, "createAudioContext"),
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
}

} // namespace audioapi
