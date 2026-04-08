#pragma once

#include <fbjni/fbjni.h>
#include <string>

namespace audioapi {

using namespace facebook;

class NativeFileInfo : public jni::JavaClass<NativeFileInfo> {
 public:
  static auto constexpr kJavaDescriptor = "Lcom/swmansion/audioapi/system/NativeFileInfo;";

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
