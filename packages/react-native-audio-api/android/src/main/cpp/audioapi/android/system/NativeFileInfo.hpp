#pragma once

#include <fbjni/fbjni.h>
#include <string>

namespace audioapi {

using namespace facebook;

/// @brief JNI bridge to Kotlin paths for recorder file output.
/// @note Call warmCache() from a JNI-attached thread
class NativeFileInfo : public jni::JavaClass<NativeFileInfo> {
 public:
  static auto constexpr kJavaDescriptor = "Lcom/swmansion/audioapi/system/NativeFileInfo;";

  /// @brief Fetches and caches directory paths via JNI. Must run before recording.
  static void warmCache();

  static std::string getFilesDir();
  static std::string getCacheDir();
};

} // namespace audioapi
