#pragma once

#include <audioapi/HostObjects/AudioContextHostObject.h>
#include <audioapi/HostObjects/OfflineAudioContextHostObject.h>
#include <audioapi/HostObjects/inputs/AudioRecorderHostObject.h>
#include <audioapi/HostObjects/utils/AudioDecoderHostObject.h>
#include <audioapi/HostObjects/utils/AudioStretcherHostObject.h>
#include <audioapi/core/AudioContext.h>
#include <audioapi/core/OfflineAudioContext.h>
#include <audioapi/core/inputs/AudioRecorder.h>
#include <audioapi/jsi/JsiPromise.h>

#include <audioapi/HostObjects/events/AudioEventHandlerRegistryHostObject.h>
#include <audioapi/events/AudioEventHandlerRegistry.h>

#include <audioapi/core/utils/worklets/SafeIncludes.h>

#include <memory>
#include <vector>

namespace audioapi {

using namespace facebook;

class AudioAPIModuleInstaller {
 private:
  inline static std::vector<std::weak_ptr<AudioContext>> contexts_ = {};

 public:
  static void injectJSIBindings(
      jsi::Runtime *jsiRuntime,
      const std::shared_ptr<react::CallInvoker> &jsCallInvoker,
      const std::shared_ptr<AudioEventHandlerRegistry>
          &audioEventHandlerRegistry,
      std::shared_ptr<worklets::WorkletRuntime> uiRuntime = nullptr) {
    auto createAudioContext = getCreateAudioContextFunction(
        jsiRuntime, jsCallInvoker, audioEventHandlerRegistry, uiRuntime);
    auto createAudioRecorder =
        getCreateAudioRecorderFunction(jsiRuntime, audioEventHandlerRegistry);
    auto createOfflineAudioContext = getCreateOfflineAudioContextFunction(
        jsiRuntime, jsCallInvoker, audioEventHandlerRegistry, uiRuntime);
    auto createAudioDecoder =
        getCreateAudioDecoderFunction(jsiRuntime, jsCallInvoker);
    auto createAudioStretcher =
        getCreateAudioStretcherFunction(jsiRuntime, jsCallInvoker);

    jsiRuntime->global().setProperty(
        *jsiRuntime, "createAudioContext", createAudioContext);
    jsiRuntime->global().setProperty(
        *jsiRuntime, "createAudioRecorder", createAudioRecorder);
    jsiRuntime->global().setProperty(
        *jsiRuntime, "createOfflineAudioContext", createOfflineAudioContext);
    jsiRuntime->global().setProperty(
        *jsiRuntime, "createAudioDecoder", createAudioDecoder);
    jsiRuntime->global().setProperty(
        *jsiRuntime, "createAudioStretcher", createAudioStretcher);

    auto audioEventHandlerRegistryHostObject =
        std::make_shared<AudioEventHandlerRegistryHostObject>(
            audioEventHandlerRegistry);
    jsiRuntime->global().setProperty(
        *jsiRuntime,
        "AudioEventEmitter",
        jsi::Object::createFromHostObject(
            *jsiRuntime, audioEventHandlerRegistryHostObject));
  }

  static void closeAllContexts() {
    for (auto it = contexts_.begin(); it != contexts_.end(); ++it) {
      auto weakContext = *it;

      if (auto context = weakContext.lock()) {
        context->close();
      }

      it = contexts_.erase(it);
    --it;
    }
  }

 private:
  static jsi::Function getCreateAudioContextFunction(
      jsi::Runtime *jsiRuntime,
      const std::shared_ptr<react::CallInvoker> &jsCallInvoker,
      const std::shared_ptr<AudioEventHandlerRegistry>
          &audioEventHandlerRegistry,
      const std::weak_ptr<worklets::WorkletRuntime> &uiRuntime) {
    return jsi::Function::createFromHostFunction(
        *jsiRuntime,
        jsi::PropNameID::forAscii(*jsiRuntime, "createAudioContext"),
        0,
        [jsCallInvoker, audioEventHandlerRegistry, uiRuntime](
            jsi::Runtime &runtime,
            const jsi::Value &thisValue,
            const jsi::Value *args,
            size_t count) -> jsi::Value {
          std::shared_ptr<AudioContext> audioContext;
          auto sampleRate = static_cast<float>(args[0].getNumber());
          auto initSuspended = args[1].getBool();

          #if RN_AUDIO_API_ENABLE_WORKLETS
              auto runtimeRegistry = RuntimeRegistry{
                  .uiRuntime = uiRuntime,
                  .audioRuntime = worklets::extractWorkletRuntime(runtime, args[2])
              };
          #else
              auto runtimeRegistry = RuntimeRegistry{};
          #endif

          audioContext = std::make_shared<AudioContext>(
              sampleRate,
              initSuspended,
              audioEventHandlerRegistry,
              runtimeRegistry);
          AudioAPIModuleInstaller::contexts_.push_back(audioContext);

          auto audioContextHostObject =
              std::make_shared<AudioContextHostObject>(
                  audioContext, &runtime, jsCallInvoker);

          return jsi::Object::createFromHostObject(
              runtime, audioContextHostObject);
        });
  }

  static jsi::Function getCreateOfflineAudioContextFunction(
      jsi::Runtime *jsiRuntime,
      const std::shared_ptr<react::CallInvoker> &jsCallInvoker,
      const std::shared_ptr<AudioEventHandlerRegistry>
          &audioEventHandlerRegistry,
      const std::weak_ptr<worklets::WorkletRuntime> &uiRuntime) {
    return jsi::Function::createFromHostFunction(
        *jsiRuntime,
        jsi::PropNameID::forAscii(*jsiRuntime, "createOfflineAudioContext"),
        0,
        [jsCallInvoker, audioEventHandlerRegistry, uiRuntime](
            jsi::Runtime &runtime,
            const jsi::Value &thisValue,
            const jsi::Value *args,
            size_t count) -> jsi::Value {
          auto numberOfChannels = static_cast<int>(args[0].getNumber());
          auto length = static_cast<size_t>(args[1].getNumber());
          auto sampleRate = static_cast<float>(args[2].getNumber());

            #if RN_AUDIO_API_ENABLE_WORKLETS
                auto runtimeRegistry = RuntimeRegistry{
                    .uiRuntime = uiRuntime,
                    .audioRuntime = worklets::extractWorkletRuntime(runtime, args[3])
                };
            #else
            auto runtimeRegistry = RuntimeRegistry{};
            #endif

            auto offlineAudioContext = std::make_shared<OfflineAudioContext>(
              numberOfChannels,
              length,
              sampleRate,
              audioEventHandlerRegistry,
              runtimeRegistry);

            auto audioContextHostObject =
              std::make_shared<OfflineAudioContextHostObject>(
                    offlineAudioContext, &runtime, jsCallInvoker);

            return jsi::Object::createFromHostObject(
                runtime, audioContextHostObject);
        });
  }

  static jsi::Function getCreateAudioRecorderFunction(
      jsi::Runtime *jsiRuntime,
      const std::shared_ptr<AudioEventHandlerRegistry>
          &audioEventHandlerRegistry) {
    return jsi::Function::createFromHostFunction(
        *jsiRuntime,
        jsi::PropNameID::forAscii(*jsiRuntime, "createAudioRecorder"),
        0,
        [audioEventHandlerRegistry](
            jsi::Runtime &runtime,
            const jsi::Value &thisValue,
            const jsi::Value *args,
            size_t count) -> jsi::Value {
          auto options = args[0].getObject(runtime);

          auto sampleRate = static_cast<float>(
              options.getProperty(runtime, "sampleRate").getNumber());
          auto bufferLength = static_cast<int>(
              options.getProperty(runtime, "bufferLengthInSamples")
                  .getNumber());

          auto audioRecorderHostObject =
              std::make_shared<AudioRecorderHostObject>(
                  audioEventHandlerRegistry, sampleRate, bufferLength);

          return jsi::Object::createFromHostObject(
              runtime, audioRecorderHostObject);
        });
  }

  static jsi::Function getCreateAudioDecoderFunction(
      jsi::Runtime *jsiRuntime,
      const std::shared_ptr<react::CallInvoker> &jsCallInvoker) {
    return jsi::Function::createFromHostFunction(
        *jsiRuntime,
        jsi::PropNameID::forAscii(*jsiRuntime, "createAudioDecoder"),
        0,
        [jsCallInvoker](
            jsi::Runtime &runtime,
            const jsi::Value &thisValue,
            const jsi::Value *args,
            size_t count) -> jsi::Value {
          auto audioDecoderHostObject =
              std::make_shared<AudioDecoderHostObject>(&runtime, jsCallInvoker);
          return jsi::Object::createFromHostObject(
              runtime, audioDecoderHostObject);
        });
  }

  static jsi::Function getCreateAudioStretcherFunction(
      jsi::Runtime *jsiRuntime,
      const std::shared_ptr<react::CallInvoker> &jsCallInvoker) {
    return jsi::Function::createFromHostFunction(
        *jsiRuntime,
        jsi::PropNameID::forAscii(*jsiRuntime, "createAudioStretcher"),
        0,
        [jsCallInvoker](
            jsi::Runtime &runtime,
            const jsi::Value &thisValue,
            const jsi::Value *args,
            size_t count) -> jsi::Value {
          auto audioStretcherHostObject =
              std::make_shared<AudioStretcherHostObject>(
                  &runtime, jsCallInvoker);
          return jsi::Object::createFromHostObject(
              runtime, audioStretcherHostObject);
        });
  }
};

} // namespace audioapi
