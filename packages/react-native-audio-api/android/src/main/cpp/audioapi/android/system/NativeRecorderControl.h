#pragma once

#include <fbjni/fbjni.h>
#include <oboe/Oboe.h>

#include <atomic>
#include <cstdint>

namespace audioapi {

using namespace facebook;

/// @brief JNI statics that let Kotlin reach the active recorder without a JS runtime,
// e.g. from the recording-notification stop action after the app task was removed.
class NativeRecorderControl : public jni::JavaClass<NativeRecorderControl> {
 public:
  static auto constexpr kJavaDescriptor = "Lcom/swmansion/audioapi/system/NativeRecorderControl;";

  static void registerNatives();

  static jint stopActiveRecording(jni::alias_ref<jni::JClass>);
  static jint pauseActiveRecording(jni::alias_ref<jni::JClass>);
  static jint resumeActiveRecording(jni::alias_ref<jni::JClass>);
  static jint currentRecorderState(jni::alias_ref<jni::JClass>);

  /// @brief The audio session id of the open capture stream, 0 when there is none. Lets
  /// Kotlin find that stream among the system's active recording configurations.
  static jint activeCaptureSessionId(jni::alias_ref<jni::JClass>);

  /// @returns The id as published: 0 when the platform allocated none (OpenSL ES, API < 28).
  static int32_t publishCaptureSession(oboe::SessionId sessionId);

  /// @brief Clears the published id only if it is still @p sessionId, so a recorder that ends
  /// late does not hide the stream that replaced it.
  static void clearCaptureSession(int32_t sessionId);

 private:
  // A plain atomic rather than a read through ActiveRecorderHandle: the main thread reads it,
  // and the handle's mutex is held while a stop finalizes the file.
  static std::atomic<int32_t> captureSessionId_;
};

} // namespace audioapi
