#include <audioapi/android/system/NativeInputRouting.h>

#include <audioapi/core/inputs/ActiveRecorderHandle.h>

namespace audioapi {

namespace {
/// Mirrors NativeInputRouting.ROUTING_FAILED on the Kotlin side.
constexpr jint kRoutingFailed = -1;
} // namespace

void NativeInputRouting::registerNatives() {
  javaClassStatic()->registerNatives({
      makeNativeMethod("rerouteActiveCapture", NativeInputRouting::rerouteActiveCapture),
  });
}

jboolean NativeInputRouting::rerouteActiveCapture(jni::alias_ref<jni::JClass> /*clazz*/) {
  return static_cast<jboolean>(ActiveRecorderHandle::global().rerouteInput().is_ok());
}

std::optional<int32_t> NativeInputRouting::acquireInputRoute() {
  jint deviceId = kRoutingFailed;
  jni::ThreadScope::WithClassLoader([&deviceId] {
    static const auto method = javaClassStatic()->getStaticMethod<jint()>("acquireInputRoute");
    deviceId = method(javaClassStatic());
  });

  if (deviceId == kRoutingFailed) {
    return std::nullopt;
  }
  return static_cast<int32_t>(deviceId);
}

void NativeInputRouting::releaseInputRoute() {
  jni::ThreadScope::WithClassLoader([] {
    static const auto method = javaClassStatic()->getStaticMethod<void()>("releaseInputRoute");
    method(javaClassStatic());
  });
}

} // namespace audioapi
