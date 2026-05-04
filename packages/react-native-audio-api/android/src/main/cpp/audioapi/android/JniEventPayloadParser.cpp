#include <audioapi/android/JniEventPayloadParser.h>
#include <string>

namespace audioapi {

using namespace facebook;

jni::local_ref<jobject> jniMapGet(
    const jni::alias_ref<jni::JMap<jstring, jobject>> &map,
    const char *key) {
  if (!map) {
    return nullptr;
  }

  static auto getMethod =
      jni::JMap<jstring, jobject>::javaClassStatic()->getMethod<jobject(jobject)>("get");

  auto jKey = jni::make_jstring(key);
  return getMethod(map, jKey.get());
}

double jniGetDouble(const jni::alias_ref<jni::JMap<jstring, jobject>> &map, const char *key) {
  auto val = jniMapGet(map, key);
  if (!val) {
    return 0.0;
  }

  if (val->isInstanceOf(jni::JDouble::javaClassStatic())) {
    return jni::static_ref_cast<jni::JDouble>(val)->value();
  }

  if (val->isInstanceOf(jni::JFloat::javaClassStatic())) {
    return static_cast<double>(jni::static_ref_cast<jni::JFloat>(val)->value());
  }

  if (val->isInstanceOf(jni::JInteger::javaClassStatic())) {
    return static_cast<double>(jni::static_ref_cast<jni::JInteger>(val)->value());
  }

  if (val->isInstanceOf(jni::JLong::javaClassStatic())) {
    return static_cast<double>(jni::static_ref_cast<jni::JLong>(val)->value());
  }

  return 0.0;
}

std::string jniGetString(const jni::alias_ref<jni::JMap<jstring, jobject>> &map, const char *key) {
  auto val = jniMapGet(map, key);
  if (val && val->isInstanceOf(jni::JString::javaClassStatic())) {
    return jni::static_ref_cast<jni::JString>(val)->toStdString();
  }

  return {};
}

bool jniGetBool(const jni::alias_ref<jni::JMap<jstring, jobject>> &map, const char *key) {
  auto val = jniMapGet(map, key);
  if (val && val->isInstanceOf(jni::JBoolean::javaClassStatic())) {
    return static_cast<bool>(jni::static_ref_cast<jni::JBoolean>(val)->value());
  }
  return false;
}

AudioEventPayload buildPayloadFromJniMap(
    AudioEvent event,
    const jni::alias_ref<jni::JMap<jstring, jobject>> &map) {
  switch (event) {
    case AudioEvent::INTERRUPTION:
      return InterruptionPayload{
          .type = jniGetString(map, "type"), .shouldResume = jniGetBool(map, "shouldResume")};

    case AudioEvent::VOLUME_CHANGE:
    case AudioEvent::PLAYBACK_NOTIFICATION_SKIP_FORWARD:
    case AudioEvent::PLAYBACK_NOTIFICATION_SKIP_BACKWARD:
    case AudioEvent::PLAYBACK_NOTIFICATION_SEEK_TO:
      return DoubleValuePayload{.value = jniGetDouble(map, "value")};

    default:
      return EmptyPayload{};
  }
}

} // namespace audioapi
