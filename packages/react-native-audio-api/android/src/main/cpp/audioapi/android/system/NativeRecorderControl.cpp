#include <audioapi/android/system/NativeRecorderControl.h>

#include <audioapi/core/inputs/ActiveRecorderHandle.h>
#include <audioapi/core/inputs/RecorderState.h>

namespace audioapi {

namespace {

jint toOrdinal(RecorderState state) {
  return static_cast<jint>(state);
}

} // namespace

void NativeRecorderControl::registerNatives() {
  javaClassStatic()->registerNatives({
      makeNativeMethod("stopActiveRecording", NativeRecorderControl::stopActiveRecording),
      makeNativeMethod("pauseActiveRecording", NativeRecorderControl::pauseActiveRecording),
      makeNativeMethod("resumeActiveRecording", NativeRecorderControl::resumeActiveRecording),
      makeNativeMethod("currentRecorderState", NativeRecorderControl::currentRecorderState),
  });
}

jint NativeRecorderControl::stopActiveRecording(jni::alias_ref<jni::JClass> /*clazz*/) {
  return toOrdinal(ActiveRecorderHandle::global().stopAndReturnState());
}

jint NativeRecorderControl::pauseActiveRecording(jni::alias_ref<jni::JClass> /*clazz*/) {
  return toOrdinal(ActiveRecorderHandle::global().pause());
}

jint NativeRecorderControl::resumeActiveRecording(jni::alias_ref<jni::JClass> /*clazz*/) {
  return toOrdinal(ActiveRecorderHandle::global().resume());
}

jint NativeRecorderControl::currentRecorderState(jni::alias_ref<jni::JClass> /*clazz*/) {
  return toOrdinal(ActiveRecorderHandle::global().currentState());
}

} // namespace audioapi
