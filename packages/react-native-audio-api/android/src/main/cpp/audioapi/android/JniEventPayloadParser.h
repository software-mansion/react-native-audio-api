#pragma once

#include <audioapi/events/AudioEvent.h>
#include <audioapi/events/AudioEventPayload.h>
#include <fbjni/fbjni.h>

namespace audioapi {

/// @brief Builds AudioEventPayload from a JNI map passed from Java/Kotlin.
AudioEventPayload buildPayloadFromJniMap(
    AudioEvent event,
    const facebook::jni::alias_ref<facebook::jni::JMap<jstring, jobject>> &map);

} // namespace audioapi
