#pragma once

#include <JsiPromise.h>
#include "AudioContext.h"
#include "AudioManager.h"
#include "AudioContextHostObject.h"
#include "AudioManagerHostObject.h"

namespace audioapi {

using namespace facebook;

class AudioAPIModuleInstaller {
public:
  static void injectJSIBindings(jsi::Runtime *jsiRuntime, const std::shared_ptr<react::CallInvoker> &jsCallInvoker) {
    auto audioManager = std::make_shared<AudioManager>();
    auto audioManagerHostObject = std::make_shared<AudioManagerHostObject>(audioManager);

    auto createAudioContext = getCreateAudioContextFunction(jsiRuntime, jsCallInvoker, audioManager);
    jsiRuntime->global().setProperty(
        *jsiRuntime, "createAudioContext", createAudioContext);
    jsiRuntime->global().setProperty(
        *jsiRuntime, "AudioManager", jsi::Object::createFromHostObject(*jsiRuntime, audioManagerHostObject));
  }

private:
  static jsi::Function getCreateAudioContextFunction(jsi::Runtime *jsiRuntime, const std::shared_ptr<react::CallInvoker> &jsCallInvoker, const std::shared_ptr<AudioManager> &audioManager) {
    return jsi::Function::createFromHostFunction(
        *jsiRuntime,
        jsi::PropNameID::forAscii(*jsiRuntime, "createAudioContext"),
        0,
        [jsiRuntime, jsCallInvoker, audioManager](
            jsi::Runtime &runtime,
            const jsi::Value &thisValue,
            const jsi::Value *args,
            size_t count) -> jsi::Value {
          std::shared_ptr<AudioContext> audioContext;
          if (args[0].isUndefined()) {
            audioContext = std::make_shared<AudioContext>(audioManager);
          } else {
            auto sampleRate = static_cast<float>(args[0].getNumber());
            audioContext = std::make_shared<AudioContext>(audioManager, sampleRate);
          }

              auto promiseVendor = std::make_shared<PromiseVendor>(jsiRuntime, jsCallInvoker);

          auto audioContextHostObject = std::make_shared<AudioContextHostObject>(
              audioContext, promiseVendor);

          return jsi::Object::createFromHostObject(
              runtime, audioContextHostObject);
        });
  }
};

} // namespace audioapi
