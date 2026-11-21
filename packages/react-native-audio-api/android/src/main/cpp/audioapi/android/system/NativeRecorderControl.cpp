#include <audioapi/android/system/NativeRecorderControl.h>

#include <audioapi/core/inputs/ActiveRecorderHandle.h>
#include <audioapi/core/inputs/RecorderState.h>

#include <algorithm>

namespace audioapi {

std::atomic<int32_t> NativeRecorderControl::captureSessionId_{0};

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
      makeNativeMethod("activeCaptureSessionId", NativeRecorderControl::activeCaptureSessionId),
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

jint NativeRecorderControl::activeCaptureSessionId(jni::alias_ref<jni::JClass> /*clazz*/) {
  return captureSessionId_.load(std::memory_order_acquire);
}

int32_t NativeRecorderControl::publishCaptureSession(oboe::SessionId sessionId) {
  const int32_t published = std::max(static_cast<int32_t>(sessionId), 0);
  captureSessionId_.store(published, std::memory_order_release);
  return published;
}

void NativeRecorderControl::clearCaptureSession(int32_t sessionId) {
  if (sessionId != 0) {
    captureSessionId_.compare_exchange_strong(sessionId, 0, std::memory_order_acq_rel);
  }
}

} // namespace audioapi
