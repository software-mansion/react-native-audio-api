#pragma once

#include <fbjni/fbjni.h>

namespace audioapi {

using namespace facebook;

class NativeAudioPlayer : public jni::JavaClass<NativeAudioPlayer> {
 public:
  static auto constexpr kJavaDescriptor = "Lcom/swmansion/audioapi/core/NativeAudioPlayer;";

  static jni::local_ref<NativeAudioPlayer> create() {
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
