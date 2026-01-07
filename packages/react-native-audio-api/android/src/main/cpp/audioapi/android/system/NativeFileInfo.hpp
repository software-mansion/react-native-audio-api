#pragma once

#include <fbjni/fbjni.h>
#include <react/jni/CxxModuleWrapper.h>
#include <react/jni/JMessageQueueThread.h>
#include <memory>
#include <utility>
#include <string>
#include <unordered_map>

namespace audioapi {

using namespace facebook;
using namespace react;

class NativeFileInfo : public jni::JavaClass<NativeFileInfo> {
 public:
  static auto constexpr kJavaDescriptor =
            "Lcom/swmansion/audioapi/system/NativeFileInfo;";

  static std::string getFilesDir() {
    static const auto method = javaClassStatic()->getStaticMethod<jni::JString()>("getFilesDir");
    return method(javaClassStatic())->toStdString();
  }

  static std::string getCacheDir() {
    static const auto method = javaClassStatic()->getStaticMethod<jni::JString()>("getCacheDir");
    return method(javaClassStatic())->toStdString();
  }
};

} // namespace audioapi
