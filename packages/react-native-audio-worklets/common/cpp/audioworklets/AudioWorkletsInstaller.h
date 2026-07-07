#pragma once

#include <audioapi/compatibility/StableAPI.h>
#include <audioworklets/HostObjects/WorkletNodeHostObject.h>
#include <worklets/Compat/StableApi.h>

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
            4,
            createWorkletNode));
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
          runtime,
          "[react-native-audio-worklets] first argument is not a valid AudioContext");
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

  static jsi::Value createWorkletNode(
      jsi::Runtime &runtime,
      const jsi::Value & /*thisValue*/,
      const jsi::Value *args,
      size_t count) {
    if (count < 4) {
      throw jsi::JSError(
          runtime,
          "[react-native-audio-worklets] __createWorkletNode expects 4 arguments");
    }
    const auto &context = getContextOrThrow(runtime, args[0]);
    auto serializableWorklet = getSerializableWorkletOrThrow(runtime, args[1]);

    auto uiRuntimeHolder = args[2].asObject(runtime);
    auto uiRuntime = worklets::getWorkletRuntimeFromHolder(runtime, uiRuntimeHolder);
    if (uiRuntime == nullptr) {
      throw jsi::JSError(
          runtime,
          "[react-native-audio-worklets] could not resolve the UI worklet runtime. "
          "Make sure react-native-worklets is installed.");
    }

    auto uiSchedulerHolder = args[3].asObject(runtime);
    auto uiScheduler = worklets::getUISchedulerFromHolder(runtime, uiSchedulerHolder);
    if (uiScheduler == nullptr) {
      throw jsi::JSError(
          runtime,
          "[react-native-audio-worklets] could not resolve the UI scheduler. "
          "Make sure react-native-worklets is installed.");
    }

    auto hostObject = std::make_shared<WorkletNodeHostObject>(
        context->getGraph(),
        context,
        std::move(uiRuntime),
        std::move(uiScheduler),
        std::move(serializableWorklet));

    auto object = jsi::Object::createFromHostObject(runtime, hostObject);
    object.setExternalMemoryPressure(runtime, hostObject->getMemoryPressure());
    return object;
  }
};

} // namespace audioworklets
