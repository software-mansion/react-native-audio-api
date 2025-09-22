#pragma once

#include <jsi/jsi.h>

#include <string>
#include <memory>

#ifdef __APPLE__
  /// We cannot make any conditional logic inside podspec but it should automatically compile those files
  /// they should be accessible if someone has react-native-worklets in node_modules
  #if __has_include(<worklets/WorkletRuntime/WorkletRuntime.h>)
    #define RN_AUDIO_API_ENABLE_WORKLETS 1
  #else
    #define RN_AUDIO_API_ENABLE_WORKLETS 0
  #endif
#endif

#ifndef RN_AUDIO_API_TEST
  #define RN_AUDIO_API_TEST 0
#endif

#if RN_AUDIO_API_ENABLE_WORKLETS
  #include <worklets/WorkletRuntime/WorkletRuntime.h>
  #include <worklets/SharedItems/Serializable.h>
  #include <worklets/NativeModules/WorkletsModuleProxy.h>
  #if ANDROID
    #include <worklets/android/WorkletsModule.h>
  #endif
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
class SerializableWorklet {
  SerializableWorklet(jsi::Runtime*, const jsi::Object &);
};
} // namespace worklets
#endif
