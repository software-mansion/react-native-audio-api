#pragma once

#include <audioapi/jsi/JsiPromise.h>
#include <audioapi/core/AudioContext.h>
#include <audioapi/core/OfflineAudioContext.h>
#include <audioapi/HostObjects/AudioContextHostObject.h>
#include <audioapi/HostObjects/OfflineAudioContextHostObject.h>

#include <memory>

namespace audioapi {

using namespace facebook;

class AudioAPIModuleInstaller {
 public:
  static void injectJSIBindings(jsi::Runtime *jsiRuntime, const std::shared_ptr<react::CallInvoker> &jsCallInvoker) {
    auto createAudioContext = getCreateAudioContextFunction(jsiRuntime, jsCallInvoker);
    auto createOfflineAudioContext = getCreateOfflineAudioContextFunction(jsiRuntime, jsCallInvoker);
    jsiRuntime->global().setProperty(
        *jsiRuntime, "createAudioContext", createAudioContext);
    jsiRuntime->global().setProperty(
        *jsiRuntime, "createOfflineAudioContext", createOfflineAudioContext);
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

          auto audioContextHostObject = std::make_shared<AudioContextHostObject>(
              audioContext, jsiRuntime, jsCallInvoker);

          return jsi::Object::createFromHostObject(
              runtime, audioContextHostObject);
        });
  }

  static jsi::Function getCreateOfflineAudioContextFunction(jsi::Runtime *jsiRuntime, const std::shared_ptr<react::CallInvoker> &jsCallInvoker) {
    return jsi::Function::createFromHostFunction(
        *jsiRuntime,
        jsi::PropNameID::forAscii(*jsiRuntime, "createOfflineAudioContext"),
        0,
        [jsiRuntime, jsCallInvoker](
            jsi::Runtime &runtime,
            const jsi::Value &thisValue,
            const jsi::Value *args,
            size_t count) -> jsi::Value {
          std::shared_ptr<OfflineAudioContext> audioContext;
          if (args[0].isObject()) {
            auto objectArg = args[0].getObject(runtime);
            int numberOfChannels = static_cast<int>(
                objectArg.getProperty(runtime, "numberOfChannels").getNumber());
            int length = static_cast<int>(
                objectArg.getProperty(runtime, "length").getNumber());
            float sampleRate = static_cast<float>(
                objectArg.getProperty(runtime, "sampleRate").getNumber());
            audioContext = std::make_shared<OfflineAudioContext>(sampleRate, length);
          } else {
            auto numberOfChannels = static_cast<int>(args[0].getNumber());
            auto length = static_cast<int>(args[1].getNumber());
            auto sampleRate = static_cast<float>(args[2].getNumber());
            audioContext = std::make_shared<OfflineAudioContext>(sampleRate, length);
          }

          auto promiseVendor = std::make_shared<PromiseVendor>(jsiRuntime, jsCallInvoker);

          auto audioContextHostObject = std::make_shared<OfflineAudioContextHostObject>(
              audioContext, promiseVendor);

          return jsi::Object::createFromHostObject(
              runtime, audioContextHostObject);
        });
  }
};

} // namespace audioapi
