#pragma once

#include <audioworklets/HostObjects/utils/NodeOptionsParser.h>
#include <audioworklets/UIWorkletsRunner.h>
#include <audioworklets/core/WorkletNodeDomain.h>
#include <audioworklets/types/NodeOptions.h>
#include <worklets/Compat/StableApi.h>
#include <worklets/WorkletRuntime/WorkletRuntime.h>

#include <jsi/jsi.h>
#include <memory>

namespace audioworklets::jsiutils {

using namespace facebook;

inline std::shared_ptr<worklets::Serializable> getSerializableWorkletOrThrow(
    jsi::Runtime &runtime,
    const jsi::Value &arg) {
  return worklets::extractSerializable(
      runtime,
      arg,
      "[react-native-audio-worklets] expected a worklet",
      worklets::Serializable::ValueType::WorkletType);
}

struct UIWorkletArgs {
  std::shared_ptr<worklets::Serializable> worklet;
  WorkletNodeDomain domain = WorkletNodeDomain::TimeDomain;
  WorkletNodeOptions options;
  std::shared_ptr<worklets::WorkletRuntime> uiRuntime;
  std::shared_ptr<worklets::UIScheduler> uiScheduler;

  [[nodiscard]] UIWorkletsRunner makeRunner() const {
    return {uiRuntime, uiScheduler, worklet};
  }
};

inline UIWorkletArgs parseUIWorkletArgs(jsi::Runtime &runtime, const jsi::Value *args) {
  UIWorkletArgs parsed;
  parsed.worklet = getSerializableWorkletOrThrow(runtime, args[0]);
  parsed.domain = option_parser::parseWorkletNodeDomain(runtime, args[1]);
  parsed.options.bufferLength = static_cast<size_t>(args[2].asNumber());
  parsed.options.smoothingTimeConstant = static_cast<float>(args[3].asNumber());

  auto uiRuntimeHolder = args[4].asObject(runtime);
  parsed.uiRuntime = worklets::getWorkletRuntimeFromHolder(runtime, uiRuntimeHolder);
  if (parsed.uiRuntime == nullptr) {
    throw jsi::JSError(
        runtime,
        "[react-native-audio-worklets] could not resolve the UI worklet runtime. "
        "Make sure react-native-worklets is installed.");
  }

  auto uiSchedulerHolder = args[5].asObject(runtime);
  parsed.uiScheduler = worklets::getUISchedulerFromHolder(runtime, uiSchedulerHolder);
  if (parsed.uiScheduler == nullptr) {
    throw jsi::JSError(
        runtime,
        "[react-native-audio-worklets] could not resolve the UI scheduler. "
        "Make sure react-native-worklets is installed.");
  }
  return parsed;
}

} // namespace audioworklets::jsiutils
