#pragma once

#include <audioapi/jsi/JsiPromise.h>
#include <audioapi/core/AudioContext.h>
#include <audioapi/core/OfflineAudioContext.h>
#include <audioapi/core/inputs/AudioRecorder.h>
#include <audioapi/HostObjects/AudioContextHostObject.h>
#include <audioapi/HostObjects/OfflineAudioContextHostObject.h>
#include <audioapi/HostObjects/AudioRecorderHostObject.h>
#include <audioapi/events/AudioEventHandlerRegistry.h>

#include <memory>

namespace audioapi {

using namespace facebook;

class AudioAPIModuleInstaller {
 public:
  static void injectJSIBindings(jsi::Runtime *jsiRuntime, const std::shared_ptr<react::CallInvoker> &jsCallInvoker) {
      auto audioEventHandlerRegistry = std::make_shared<AudioEventHandlerRegistry>(jsiRuntime, jsCallInvoker);

    auto createAudioContext = getCreateAudioContextFunction(jsiRuntime, jsCallInvoker, audioEventHandlerRegistry);
    auto createAudioRecorder = getCreateAudioRecorderFunction(jsiRuntime, jsCallInvoker);
    auto createOfflineAudioContext = getCreateOfflineAudioContextFunction(jsiRuntime, jsCallInvoker, audioEventHandlerRegistry);

    jsiRuntime->global().setProperty(*jsiRuntime, "createAudioContext", createAudioContext);
    jsiRuntime->global().setProperty(*jsiRuntime, "createAudioRecorder", createAudioRecorder);
    jsiRuntime->global().setProperty(*jsiRuntime, "createOfflineAudioContext", createOfflineAudioContext);
  }

 private:
  static jsi::Function getCreateAudioContextFunction(jsi::Runtime *jsiRuntime, const std::shared_ptr<react::CallInvoker> &jsCallInvoker, const std::shared_ptr<AudioEventHandlerRegistry> &eventHandlerRegistry) {
    return jsi::Function::createFromHostFunction(
        *jsiRuntime,
        jsi::PropNameID::forAscii(*jsiRuntime, "createAudioContext"),
        0,
        [jsCallInvoker, eventHandlerRegistry](
            jsi::Runtime &runtime,
            const jsi::Value &thisValue,
            const jsi::Value *args,
            size_t count) -> jsi::Value {
          std::shared_ptr<AudioContext> audioContext;
          auto sampleRate = static_cast<float>(args[0].getNumber());
          audioContext = std::make_shared<AudioContext>(sampleRate, eventHandlerRegistry);

          auto audioContextHostObject = std::make_shared<AudioContextHostObject>(
              audioContext, &runtime, jsCallInvoker);

          return jsi::Object::createFromHostObject(
              runtime, audioContextHostObject);
        });
  }

  static jsi::Function getCreateOfflineAudioContextFunction(jsi::Runtime *jsiRuntime, const std::shared_ptr<react::CallInvoker> &jsCallInvoker, const std::shared_ptr<AudioEventHandlerRegistry> &eventHandlerRegistry) {
    return jsi::Function::createFromHostFunction(
        *jsiRuntime,
        jsi::PropNameID::forAscii(*jsiRuntime, "createOfflineAudioContext"),
        0,
        [jsCallInvoker, eventHandlerRegistry](
            jsi::Runtime &runtime,
            const jsi::Value &thisValue,
            const jsi::Value *args,
            size_t count) -> jsi::Value {
            auto numberOfChannels = static_cast<int>(args[0].getNumber());
            auto length = static_cast<size_t>(args[1].getNumber());
            auto sampleRate = static_cast<float>(args[2].getNumber());

          auto offlineAudioContext = std::make_shared<OfflineAudioContext>(numberOfChannels, length, sampleRate, eventHandlerRegistry);
          auto audioContextHostObject = std::make_shared<OfflineAudioContextHostObject>(
              offlineAudioContext, &runtime, jsCallInvoker);

          return jsi::Object::createFromHostObject(
              runtime, audioContextHostObject);
        });
  }

  static jsi::Function getCreateAudioRecorderFunction(jsi::Runtime *jsiRuntime, const std::shared_ptr<react::CallInvoker> &jsCallInvoker) {
    return jsi::Function::createFromHostFunction(
        *jsiRuntime,
        jsi::PropNameID::forAscii(*jsiRuntime, "createAudioRecorder"),
        0,
        [jsCallInvoker](
            jsi::Runtime &runtime,
            const jsi::Value &thisValue,
            const jsi::Value *args,
            size_t count) -> jsi::Value {
          auto options = args[0].getObject(runtime);

          auto sampleRate = static_cast<float>(options.getProperty(runtime, "sampleRate").getNumber());
          auto bufferLength = static_cast<int>(options.getProperty(runtime, "bufferLengthInSamples").getNumber());

          auto audioRecorderHostObject = std::make_shared<AudioRecorderHostObject>(&runtime, jsCallInvoker, sampleRate, bufferLength);

          return jsi::Object::createFromHostObject(runtime, audioRecorderHostObject);
        });
  }
};

} // namespace audioapi
