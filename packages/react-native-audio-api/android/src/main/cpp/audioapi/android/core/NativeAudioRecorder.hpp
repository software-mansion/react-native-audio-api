#pragma once

#include <fbjni/fbjni.h>

namespace audioapi {

using namespace facebook;

class NativeAudioRecorder : public jni::JavaClass<NativeAudioRecorder> {
 public:
  static auto constexpr kJavaDescriptor = "Lcom/swmansion/audioapi/core/NativeAudioRecorder;";

  static jni::local_ref<NativeAudioRecorder> create() {
    return newInstance();
  }

  void start() {
    static const auto method = javaClassStatic()->getMethod<void()>("start");
    method(self());
  }

  void stop() {
    static const auto method = javaClassStatic()->getMethod<void()>("stop");
    method(self());
  }
};

} // namespace audioapi
