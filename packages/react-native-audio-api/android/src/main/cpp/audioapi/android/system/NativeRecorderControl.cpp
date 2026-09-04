#include <audioapi/android/system/NativeRecorderControl.hpp>

#include <audioapi/core/inputs/ActiveRecorderHandle.h>

namespace audioapi {

void NativeRecorderControl::registerNatives() {
  javaClassStatic()->registerNatives({
      makeNativeMethod("stopActiveRecording", NativeRecorderControl::stopActiveRecording),
      makeNativeMethod("pauseActiveRecording", NativeRecorderControl::pauseActiveRecording),
      makeNativeMethod("resumeActiveRecording", NativeRecorderControl::resumeActiveRecording),
      makeNativeMethod("isRecordingOngoing", NativeRecorderControl::isRecordingOngoing),
  });
}

jboolean NativeRecorderControl::stopActiveRecording(jni::alias_ref<jni::JClass> /*clazz*/) {
  return static_cast<jboolean>(ActiveRecorderHandle::global().stopActiveRecording());
}

jboolean NativeRecorderControl::pauseActiveRecording(jni::alias_ref<jni::JClass> /*clazz*/) {
  return static_cast<jboolean>(ActiveRecorderHandle::global().pauseActiveRecording());
}

jboolean NativeRecorderControl::resumeActiveRecording(jni::alias_ref<jni::JClass> /*clazz*/) {
  return static_cast<jboolean>(ActiveRecorderHandle::global().resumeActiveRecording());
}

jboolean NativeRecorderControl::isRecordingOngoing(jni::alias_ref<jni::JClass> /*clazz*/) {
  return static_cast<jboolean>(ActiveRecorderHandle::global().isRecordingOngoing());
}

} // namespace audioapi
