#pragma once

#include <jsi/jsi.h>

#include <string>
#include <memory>

#if RN_AUDIO_API_ENABLE_WORKLETS
#include <worklets/WorkletRuntime/WorkletRuntime.h>
#include <worklets/SharedItems/Shareables.h>
#include <worklets/NativeModules/WorkletsModuleProxy.h>
#include <worklets/android/WorkletsModule.h>
#else
/// @brief Dummy implementation of worklets for non-worklet builds they should do nothing and mock necessary methods
/// @note It helps to reduce compile time branching across codebase
/// @note If you need to base some c++ implementation on if the worklets are enabled use `#if RN_AUDIO_API_ENABLE_WORKLETS`
namespace worklets {

using namespace facebook;
class MessageQueueThread {};
class WorkletsModuleProxy {};
class WorkletRuntime {
  explicit WorkletRuntime(uint64_t, const std::shared_ptr<MessageQueueThread> &, const std::string &, const bool);
};
class ShareableWorklet {
  ShareableWorklet(jsi::Runtime*, const jsi::Object &);
};
} // namespace worklets
#endif
