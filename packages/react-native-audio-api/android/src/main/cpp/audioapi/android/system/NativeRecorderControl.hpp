#pragma once

#include <fbjni/fbjni.h>

namespace audioapi {

using namespace facebook;

/// @brief JNI statics that let Kotlin reach the active recorder without a React
/// context or JS runtime, e.g. from the recording-notification stop action after
/// the app task was removed. Backed by ActiveRecorderHandle.
class NativeRecorderControl : public jni::JavaClass<NativeRecorderControl> {
 public:
  static auto constexpr kJavaDescriptor = "Lcom/swmansion/audioapi/system/NativeRecorderControl;";

  static void registerNatives();

  static jboolean stopActiveRecording(jni::alias_ref<jni::JClass>);
  static jboolean pauseActiveRecording(jni::alias_ref<jni::JClass>);
  static jboolean resumeActiveRecording(jni::alias_ref<jni::JClass>);
  static jboolean isRecordingActive(jni::alias_ref<jni::JClass>);
};

} // namespace audioapi
