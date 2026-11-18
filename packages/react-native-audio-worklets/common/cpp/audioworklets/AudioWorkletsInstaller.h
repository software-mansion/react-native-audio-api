#pragma once

#include <audioapi/compatibility/StableAPI.h>
#include <audioworklets/HostObjects/RecorderWorkletHostObject.h>
#include <audioworklets/HostObjects/WorkletNodeHostObject.h>
#include <audioworklets/HostObjects/WorkletProcessingNodeHostObject.h>
#include <audioworklets/HostObjects/WorkletSourceNodeHostObject.h>
#include <audioworklets/jsi/JsiUtils.h>
#include <worklets/Compat/StableApi.h>
#include <worklets/WorkletRuntime/WorkletRuntime.h>

#include <jsi/jsi.h>
#include <memory>
#include <utility>

namespace audioworklets {

using namespace facebook;

class AudioWorkletsInstaller {
 public:
  static void inject(jsi::Runtime &runtime) {
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
        "__createRecorderWorklet",
        jsi::Function::createFromHostFunction(
            runtime,
            jsi::PropNameID::forAscii(runtime, "__createRecorderWorklet"),
            6,
            createRecorderWorklet));

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
    const auto parsed = jsiutils::parseUIWorkletArgs(runtime, args + 1);

    auto hostObject = std::make_shared<WorkletNodeHostObject>(
        context->getGraph(),
        context,
        parsed.uiRuntime,
        parsed.uiScheduler,
        parsed.worklet,
        parsed.domain,
        parsed.options);

    auto object = jsi::Object::createFromHostObject(runtime, hostObject);
    object.setExternalMemoryPressure(runtime, hostObject->getMemoryPressure());
    return object;
  }

  static jsi::Value createRecorderWorklet(
      jsi::Runtime &runtime,
      const jsi::Value & /*thisValue*/,
      const jsi::Value *args,
      size_t count) {
    if (count < 6) {
      throw jsi::JSError(
          runtime, "[react-native-audio-worklets] __createRecorderWorklet expects 6 arguments");
    }
    const auto parsed = jsiutils::parseUIWorkletArgs(runtime, args);

    auto worklet =
        std::make_shared<RecorderWorklet>(parsed.makeRunner(), parsed.domain, parsed.options);
    return jsi::Object::createFromHostObject(
        runtime, std::make_shared<RecorderWorkletHostObject>(std::move(worklet)));
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
    auto serializableWorklet = jsiutils::getSerializableWorkletOrThrow(runtime, args[1]);
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
    auto serializableWorklet = jsiutils::getSerializableWorkletOrThrow(runtime, args[1]);
    const auto workletRuntime = getAudioRuntimeOrThrow(runtime, args[2]);

    auto hostObject = std::make_shared<WorkletProcessingNodeHostObject>(
        context->getGraph(), context, workletRuntime, serializableWorklet);

    auto object = jsi::Object::createFromHostObject(runtime, hostObject);
    object.setExternalMemoryPressure(runtime, hostObject->getMemoryPressure());
    return object;
  }
};

} // namespace audioworklets
