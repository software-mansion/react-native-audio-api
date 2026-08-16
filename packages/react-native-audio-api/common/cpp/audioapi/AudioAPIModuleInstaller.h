#pragma once

#include <audioapi/HostObjects/AudioContextHostObject.h>
#include <audioapi/HostObjects/OfflineAudioContextHostObject.h>
#include <audioapi/HostObjects/inputs/AudioRecorderHostObject.h>
#include <audioapi/HostObjects/sources/AudioBufferHostObject.h>
#include <audioapi/HostObjects/utils/AudioDecoderHostObject.h>
#include <audioapi/HostObjects/utils/AudioFileUtilsHostObject.h>
#include <audioapi/core/AudioContext.h>
#include <audioapi/core/OfflineAudioContext.h>
#include <audioapi/core/inputs/AudioRecorder.h>
#include <audioapi/jsi/JsiPromise.h>
#include <audioapi/utils/AudioBuffer.hpp>

#include <audioapi/HostObjects/events/AudioEventHandlerRegistryHostObject.h>
#include <audioapi/events/IAudioEventHandlerRegistry.h>

#include <memory>
namespace audioapi {

using namespace facebook;

class AudioAPIModuleInstaller {
 public:
  static void injectJSIBindings(
      jsi::Runtime *jsiRuntime,
      const std::shared_ptr<react::CallInvoker> &jsCallInvoker,
      const std::shared_ptr<IAudioEventHandlerRegistry> &audioEventHandlerRegistry) {
    auto createAudioContext =
        getCreateAudioContextFunction(jsiRuntime, jsCallInvoker, audioEventHandlerRegistry);
    auto createAudioRecorder =
        getCreateAudioRecorderFunction(jsiRuntime, jsCallInvoker, audioEventHandlerRegistry);
    auto createOfflineAudioContext =
        getCreateOfflineAudioContextFunction(jsiRuntime, jsCallInvoker, audioEventHandlerRegistry);
    auto createAudioBuffer = getCreateAudioBufferFunction(jsiRuntime);
    auto createAudioDecoder = getCreateAudioDecoderFunction(jsiRuntime, jsCallInvoker);
    auto createAudioFileUtils = getCreateAudioFileUtilsFunction(jsiRuntime, jsCallInvoker);

    jsiRuntime->global().setProperty(*jsiRuntime, "createAudioContext", createAudioContext);
    jsiRuntime->global().setProperty(*jsiRuntime, "createAudioRecorder", createAudioRecorder);
    jsiRuntime->global().setProperty(
        *jsiRuntime, "createOfflineAudioContext", createOfflineAudioContext);
    jsiRuntime->global().setProperty(*jsiRuntime, "createAudioBuffer", createAudioBuffer);
    jsiRuntime->global().setProperty(*jsiRuntime, "createAudioDecoder", createAudioDecoder);
    jsiRuntime->global().setProperty(*jsiRuntime, "createAudioFileUtils", createAudioFileUtils);

    auto audioEventHandlerRegistryHostObject =
        std::make_shared<AudioEventHandlerRegistryHostObject>(audioEventHandlerRegistry);
    jsiRuntime->global().setProperty(
        *jsiRuntime,
        "AudioEventEmitter",
        jsi::Object::createFromHostObject(*jsiRuntime, audioEventHandlerRegistryHostObject));
  }

 private:
  static jsi::Function getCreateAudioContextFunction(
      jsi::Runtime *jsiRuntime,
      const std::shared_ptr<react::CallInvoker> &jsCallInvoker,
      const std::shared_ptr<IAudioEventHandlerRegistry> &audioEventHandlerRegistry) {
    return jsi::Function::createFromHostFunction(
        *jsiRuntime,
        jsi::PropNameID::forAscii(*jsiRuntime, "createAudioContext"),
        2,
        [jsCallInvoker, audioEventHandlerRegistry](
            jsi::Runtime &runtime,
            const jsi::Value &thisValue,
            const jsi::Value *args,
            size_t count) -> jsi::Value {
          auto sampleRate = static_cast<float>(args[0].getNumber());

          // Unknown strings fall back to INTERACTIVE, matching how browsers
          // treat an unrecognised latencyHint.
          auto latencyHint = AudioContextLatencyHint::INTERACTIVE;
          if (count > 1 && args[1].isString()) {
            auto hint = args[1].getString(runtime).utf8(runtime);
            if (hint == "balanced") {
              latencyHint = AudioContextLatencyHint::BALANCED;
            } else if (hint == "playback") {
              latencyHint = AudioContextLatencyHint::PLAYBACK;
            }
          }

          auto audioContextHostObject = std::make_shared<AudioContextHostObject>(
              sampleRate, audioEventHandlerRegistry, &runtime, jsCallInvoker, latencyHint);

          return jsi::Object::createFromHostObject(runtime, audioContextHostObject);
        });
  }

  static jsi::Function getCreateOfflineAudioContextFunction(
      jsi::Runtime *jsiRuntime,
      const std::shared_ptr<react::CallInvoker> &jsCallInvoker,
      const std::shared_ptr<IAudioEventHandlerRegistry> &audioEventHandlerRegistry) {
    return jsi::Function::createFromHostFunction(
        *jsiRuntime,
        jsi::PropNameID::forAscii(*jsiRuntime, "createOfflineAudioContext"),
        3,
        [jsCallInvoker, audioEventHandlerRegistry](
            jsi::Runtime &runtime,
            const jsi::Value &thisValue,
            const jsi::Value *args,
            size_t count) -> jsi::Value {
          auto numberOfChannels = static_cast<int>(args[0].getNumber());
          auto length = static_cast<size_t>(args[1].getNumber());
          auto sampleRate = static_cast<float>(args[2].getNumber());

          auto audioContextHostObject = std::make_shared<OfflineAudioContextHostObject>(
              numberOfChannels,
              length,
              sampleRate,
              audioEventHandlerRegistry,
              &runtime,
              jsCallInvoker);

          return jsi::Object::createFromHostObject(runtime, audioContextHostObject);
        });
  }

  static jsi::Function getCreateAudioRecorderFunction(
      jsi::Runtime *jsiRuntime,
      const std::shared_ptr<react::CallInvoker> &jsCallInvoker,
      const std::shared_ptr<IAudioEventHandlerRegistry> &audioEventHandlerRegistry) {
    return jsi::Function::createFromHostFunction(
        *jsiRuntime,
        jsi::PropNameID::forAscii(*jsiRuntime, "createAudioRecorder"),
        0,
        [jsCallInvoker, audioEventHandlerRegistry](
            jsi::Runtime &runtime,
            const jsi::Value &thisValue,
            const jsi::Value *args,
            size_t count) -> jsi::Value {
          auto audioRecorderHostObject = std::make_shared<AudioRecorderHostObject>(
              audioEventHandlerRegistry, &runtime, jsCallInvoker);

          auto jsiObject = jsi::Object::createFromHostObject(runtime, audioRecorderHostObject);

          return jsiObject;
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
          return jsi::Object::createFromHostObject(runtime, audioDecoderHostObject);
        });
  }

  static jsi::Function getCreateAudioFileUtilsFunction(
      jsi::Runtime *jsiRuntime,
      const std::shared_ptr<react::CallInvoker> &jsCallInvoker) {
    return jsi::Function::createFromHostFunction(
        *jsiRuntime,
        jsi::PropNameID::forAscii(*jsiRuntime, "createAudioFileUtils"),
        0,
        [jsCallInvoker](
            jsi::Runtime &runtime,
            const jsi::Value &thisValue,
            const jsi::Value *args,
            size_t count) -> jsi::Value {
          auto audioFileUtilsHostObject =
              std::make_shared<AudioFileUtilsHostObject>(&runtime, jsCallInvoker);
          return jsi::Object::createFromHostObject(runtime, audioFileUtilsHostObject);
        });
  }

  static jsi::Function getCreateAudioBufferFunction(jsi::Runtime *jsiRuntime) {
    return jsi::Function::createFromHostFunction(
        *jsiRuntime,
        jsi::PropNameID::forAscii(*jsiRuntime, "createAudioBuffer"),
        3,
        [](jsi::Runtime &runtime, const jsi::Value &thisValue, const jsi::Value *args, size_t count)
            -> jsi::Value {
          auto numberOfChannels = static_cast<int>(args[0].getNumber());
          auto length = static_cast<size_t>(args[1].getNumber());
          auto sampleRate = static_cast<float>(args[2].getNumber());

          auto audioBuffer = std::make_shared<AudioBuffer>(length, numberOfChannels, sampleRate);
          auto audioBufferHostObject = std::make_shared<AudioBufferHostObject>(audioBuffer);
          return jsi::Object::createFromHostObject(runtime, audioBufferHostObject);
        });
  }
};

} // namespace audioapi
