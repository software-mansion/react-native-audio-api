#pragma once

#include <jsi/jsi.h>

#include <string>
#include <memory>

#if ANDROID
  #include <fbjni/detail/Environment.h>
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

#define RN_AUDIO_API_WORKLETS_DISABLED_ERROR \
  std::runtime_error(               \
      "Worklets are disabled. Please install react-native-worklets or check if you have supported version to enable these features.");

/// @brief Dummy implementation of worklets for non-worklet builds they should do nothing and mock necessary methods
/// @note It helps to reduce compile time branching across codebase
/// @note If you need to base some c++ implementation on if the worklets are enabled use `#if RN_AUDIO_API_ENABLE_WORKLETS`
namespace worklets {

using namespace facebook;
class MessageQueueThread {};
class WorkletsModuleProxy {};
class WorkletRuntime {
 public:
  explicit WorkletRuntime(uint64_t, const std::shared_ptr<MessageQueueThread> &, const std::string &, const bool) {
    throw RN_AUDIO_API_WORKLETS_DISABLED_ERROR
  }
  jsi::Runtime &getJSIRuntime() const {
    throw RN_AUDIO_API_WORKLETS_DISABLED_ERROR
  }
  jsi::Value executeSync(jsi::Runtime &rt, const jsi::Value &worklet) const {
    throw RN_AUDIO_API_WORKLETS_DISABLED_ERROR
  }
  jsi::Value executeSync(std::function<jsi::Value(jsi::Runtime &)> &&job) const {
    throw RN_AUDIO_API_WORKLETS_DISABLED_ERROR
  }
  jsi::Value executeSync(const std::function<jsi::Value(jsi::Runtime &)> &job) const {
    throw RN_AUDIO_API_WORKLETS_DISABLED_ERROR
  }
};
class SerializableWorklet {
 public:
  SerializableWorklet(jsi::Runtime*, const jsi::Object &) {
    throw RN_AUDIO_API_WORKLETS_DISABLED_ERROR
  }
  jsi::Value toJSValue(jsi::Runtime &rt) {
    throw RN_AUDIO_API_WORKLETS_DISABLED_ERROR
  }
};
} // namespace worklets

#undef RN_AUDIO_API_WORKLETS_DISABLED_ERROR

#endif

/// @brief Struct to hold references to different runtimes used in the AudioAPI
/// @note it is used to pass them around and avoid creating multiple instances of the same runtime
struct RuntimeRegistry {
  std::weak_ptr<worklets::WorkletRuntime> uiRuntime;
  std::shared_ptr<worklets::WorkletRuntime> audioRuntime;

#if ANDROID
    ~RuntimeRegistry() {
        facebook::jni::ThreadScope::WithClassLoader(
            [this]() {
                uiRuntime.reset();
                audioRuntime.reset();
            });
    }
#endif
};
