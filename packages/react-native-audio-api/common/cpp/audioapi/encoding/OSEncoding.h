#pragma once

#include <audioapi/encoding/AudioEncoder.h>
#include <audioapi/utils/AudioFileProperties.h>

#include <memory>

#if defined(__ANDROID__)
#include <audioapi/android/AndroidEncoder.h>
#define RN_AUDIO_API_HAS_OS_ENCODER 1
namespace audioapi::os_encoder {
using Encoder = android_encoder::AndroidEncoder;
} // namespace audioapi::os_encoder
#elif defined(__APPLE__) && !defined(RN_AUDIO_API_TEST)
#include <audioapi/ios/core/utils/IOSEncoder.h>
#define RN_AUDIO_API_HAS_OS_ENCODER 1
namespace audioapi::os_encoder {
using Encoder = ios_encoder::IOSEncoder;
} // namespace audioapi::os_encoder
#else
#define RN_AUDIO_API_HAS_OS_ENCODER 0
#endif

namespace audioapi {

/// Returns nullptr on platforms without a system encoder (e.g. the desktop test build).
inline std::unique_ptr<AudioEncoder> createOsEncoder(
    const std::shared_ptr<AudioFileProperties> &fileProperties) {
#if RN_AUDIO_API_HAS_OS_ENCODER
  return std::make_unique<os_encoder::Encoder>(fileProperties);
#else
  (void)fileProperties;
  return nullptr;
#endif
}

} // namespace audioapi
