#pragma once

#include <JsiPromise.h>
#include "AudioContext.h"
#include "AudioContextHostObject.h"

namespace audioapi {

using namespace facebook;

class AudioAPIModuleInstaller {
public:
  static void injectJSIBindings(jsi::Runtime *jsiRuntime, const std::shared_ptr<react::CallInvoker> &jsCallInvoker) {
    auto createAudioContext = getCreateAudioContextFunction(jsiRuntime, jsCallInvoker);
    jsiRuntime->global().setProperty(
        *jsiRuntime, "createAudioContext", createAudioContext);
  }

private:
  static jsi::Function getCreateAudioContextFunction(jsi::Runtime *jsiRuntime, const std::shared_ptr<react::CallInvoker> &jsCallInvoker) {
    return jsi::Function::createFromHostFunction(
        *jsiRuntime,
        jsi::PropNameID::forAscii(*jsiRuntime, "createAudioContext"),
        0,
        [jsiRuntime, jsCallInvoker](
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
              
              auto promiseVendor = std::make_shared<PromiseVendor>(jsiRuntime, jsCallInvoker);

          auto audioContextHostObject = std::make_shared<AudioContextHostObject>(
              audioContext, promiseVendor);

          return jsi::Object::createFromHostObject(
              runtime, audioContextHostObject);
        });
  }
};

} // namespace audioapi
