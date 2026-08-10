#pragma once

#include <audioapi/compatibility/StableAPI.h>
#include <audioworklets/HostObjects/WorkletAudioContextHostObject.h>
#include <audioworklets/HostObjects/WorkletNodeHostObject.h>
#include <audioworklets/HostObjects/WorkletProcessingNodeHostObject.h>
#include <audioworklets/HostObjects/WorkletSourceNodeHostObject.h>
#include <audioworklets/HostObjects/utils/NodeOptionsParser.h>
#include <worklets/Compat/StableApi.h>
#include <worklets/WorkletRuntime/WorkletRuntime.h>

#include <audioapi/HostObjects/events/AudioEventHandlerRegistryHostObject.h>
#include <audioapi/events/IAudioEventHandlerRegistry.h>

#include <jsi/jsi.h>
#include <memory>
#include <utility>

namespace audioworklets {

using namespace facebook;

class AudioWorkletsInstaller {
 public:
  static void inject(
      jsi::Runtime &runtime,
      const std::shared_ptr<react::CallInvoker> &callInvoker) {
    runtime.global().setProperty(
        runtime,
        "__createWorkletAudioContext",
        jsi::Function::createFromHostFunction(
            runtime,
            jsi::PropNameID::forAscii(runtime, "__createWorkletAudioContext"),
            1,
            [callInvoker](
                jsi::Runtime &rt,
                const jsi::Value & /*thisValue*/,
                const jsi::Value *args,
                size_t count) -> jsi::Value {
              return createWorkletAudioContext(rt, callInvoker, args, count);
            }));

    runtime.global().setProperty(
        runtime,
        "__createWorkletNode",
        jsi::Function::createFromHostFunction(
            runtime,
            jsi::PropNameID::forAscii(runtime, "__createWorkletNode"),
            7,
            createWorkletNode));

    runtime.global().setProperty(
        runtime,
        "__createWorkletSourceNode",
        jsi::Function::createFromHostFunction(
            runtime,
            jsi::PropNameID::forAscii(runtime, "__createWorkletSourceNode"),
            3,
            createWorkletSourceNode));

    runtime.global().setProperty(
        runtime,
        "__createWorkletProcessingNode",
        jsi::Function::createFromHostFunction(
            runtime,
            jsi::PropNameID::forAscii(runtime, "__createWorkletProcessingNode"),
            3,
            createWorkletProcessingNode));
  }

 private:
  static std::shared_ptr<audioapi::IAudioEventHandlerRegistry> getAudioEventHandlerRegistryOrThrow(
      jsi::Runtime &runtime) {
    auto emitter = runtime.global().getProperty(runtime, "AudioEventEmitter");
    if (!emitter.isObject()) {
      throw jsi::JSError(
          runtime,
          "[react-native-audio-worklets] AudioEventEmitter is not installed. "
          "Make sure react-native-audio-api is initialized before audio-worklets.");
    }

    auto hostObject =
        emitter.asObject(runtime).getHostObject<audioapi::AudioEventHandlerRegistryHostObject>(
            runtime);
    if (hostObject == nullptr) {
      throw jsi::JSError(
          runtime,
          "[react-native-audio-worklets] AudioEventEmitter is not a valid audio event registry");
    }

    return hostObject->getEventHandlerRegistry();
  }

  static jsi::Value createWorkletAudioContext(
      jsi::Runtime &runtime,
      const std::shared_ptr<react::CallInvoker> &callInvoker,
      const jsi::Value *args,
      size_t count) {
    if (count < 1) {
      throw jsi::JSError(
          runtime, "[react-native-audio-worklets] __createWorkletAudioContext expects 1 argument");
    }

    const auto sampleRate = static_cast<float>(args[0].asNumber());
    const auto audioEventHandlerRegistry = getAudioEventHandlerRegistryOrThrow(runtime);

    auto hostObject = std::make_shared<WorkletAudioContextHostObject>(
        sampleRate, audioEventHandlerRegistry, &runtime, callInvoker);

    return jsi::Object::createFromHostObject(runtime, hostObject);
  }

  static std::shared_ptr<audioapi::BaseAudioContext> getContextOrThrow(
      jsi::Runtime &runtime,
      const jsi::Value &arg) {
    auto contextObject = arg.asObject(runtime);
    auto contextHostObject =
        contextObject.getHostObject<audioapi::BaseAudioContextHostObject>(runtime);
    if (contextHostObject == nullptr) {
      throw jsi::JSError(
          runtime, "[react-native-audio-worklets] first argument is not a valid AudioContext");
    }

    return contextHostObject->getContext();
  }

  static std::shared_ptr<worklets::Serializable> getSerializableWorkletOrThrow(
      jsi::Runtime &runtime,
      const jsi::Value &arg) {
    return worklets::extractSerializable(
        runtime,
        arg,
        "[react-native-audio-worklets] expected a worklet as the second argument",
        worklets::Serializable::ValueType::WorkletType);
  }

  static std::weak_ptr<worklets::WorkletRuntime> getAudioRuntimeOrThrow(
      jsi::Runtime &runtime,
      const jsi::Value &arg) {
    return worklets::extractWorkletRuntime(runtime, arg);
  }

  static jsi::Value createWorkletNode(
      jsi::Runtime &runtime,
      const jsi::Value & /*thisValue*/,
      const jsi::Value *args,
      size_t count) {
    if (count < 7) {
      throw jsi::JSError(
          runtime, "[react-native-audio-worklets] __createWorkletNode expects 7 arguments");
    }
    const auto &context = getContextOrThrow(runtime, args[0]);
    auto serializableWorklet = getSerializableWorkletOrThrow(runtime, args[1]);
    const auto domain = option_parser::parseWorkletNodeDomain(runtime, args[2]);

    WorkletNodeOptions options;
    options.bufferLength = static_cast<size_t>(args[3].asNumber());
    options.smoothingTimeConstant = static_cast<float>(args[4].asNumber());

    auto uiRuntimeHolder = args[5].asObject(runtime);
    auto uiRuntime = worklets::getWorkletRuntimeFromHolder(runtime, uiRuntimeHolder);
    if (uiRuntime == nullptr) {
      throw jsi::JSError(
          runtime,
          "[react-native-audio-worklets] could not resolve the UI worklet runtime. "
          "Make sure react-native-worklets is installed.");
    }

    auto uiSchedulerHolder = args[6].asObject(runtime);
    auto uiScheduler = worklets::getUISchedulerFromHolder(runtime, uiSchedulerHolder);
    if (uiScheduler == nullptr) {
      throw jsi::JSError(
          runtime,
          "[react-native-audio-worklets] could not resolve the UI scheduler. "
          "Make sure react-native-worklets is installed.");
    }

    auto hostObject = std::make_shared<WorkletNodeHostObject>(
        context->getGraph(), context, uiRuntime, uiScheduler, serializableWorklet, domain, options);

    auto object = jsi::Object::createFromHostObject(runtime, hostObject);
    object.setExternalMemoryPressure(runtime, hostObject->getMemoryPressure());
    return object;
  }

  static jsi::Value createWorkletSourceNode(
      jsi::Runtime &runtime,
      const jsi::Value & /*thisValue*/,
      const jsi::Value *args,
      size_t count) {
    if (count < 3) {
      throw jsi::JSError(
          runtime, "[react-native-audio-worklets] __createWorkletSourceNode expects 3 arguments");
    }

    const auto &context = getContextOrThrow(runtime, args[0]);
    auto serializableWorklet = getSerializableWorkletOrThrow(runtime, args[1]);
    const auto workletRuntime = getAudioRuntimeOrThrow(runtime, args[2]);

    auto hostObject = std::make_shared<WorkletSourceNodeHostObject>(
        context->getGraph(), context, workletRuntime, serializableWorklet);

    return jsi::Object::createFromHostObject(runtime, hostObject);
  }

  static jsi::Value createWorkletProcessingNode(
      jsi::Runtime &runtime,
      const jsi::Value & /*thisValue*/,
      const jsi::Value *args,
      size_t count) {
    if (count < 3) {
      throw jsi::JSError(
          runtime,
          "[react-native-audio-worklets] __createWorkletProcessingNode expects 3 arguments");
    }

    const auto &context = getContextOrThrow(runtime, args[0]);
    auto serializableWorklet = getSerializableWorkletOrThrow(runtime, args[1]);
    const auto workletRuntime = getAudioRuntimeOrThrow(runtime, args[2]);

    auto hostObject = std::make_shared<WorkletProcessingNodeHostObject>(
        context->getGraph(), context, workletRuntime, serializableWorklet);

    auto object = jsi::Object::createFromHostObject(runtime, hostObject);
    object.setExternalMemoryPressure(runtime, hostObject->getMemoryPressure());
    return object;
  }
};

} // namespace audioworklets
