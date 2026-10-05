#pragma once

#include <audioapi/encoding/AudioEncoder.h>
#include <audioapi/utils/AudioFileProperties.h>

#include <memory>

#if defined(__ANDROID__)
#include <audioapi/android/AndroidEncoder.h>
#define RN_AUDIO_API_HAS_OS_ENCODER 1
namespace audioapi::os_encoder {
using Encoder = android::encoder::AndroidEncoder;
} // namespace audioapi::os_encoder
#elif defined(__APPLE__) && !defined(RN_AUDIO_API_TEST) && !defined(RN_AUDIO_API_NODE)
#include <audioapi/ios/core/utils/IOSEncoder.h>
#define RN_AUDIO_API_HAS_OS_ENCODER 1
namespace audioapi::os_encoder {
using Encoder = ios::encoder::IOSEncoder;
} // namespace audioapi::os_encoder
#else
#define RN_AUDIO_API_HAS_OS_ENCODER 0
#endif

namespace audioapi {

/// Fails on platforms without a system encoder (e.g. the desktop test build).
inline CreateEncoderResult createOsEncoder(const EncoderSettings &settings) {
#if RN_AUDIO_API_HAS_OS_ENCODER
  return CreateEncoderResult::Ok(std::make_unique<os_encoder::Encoder>(settings));
#else
  (void)settings;
  return CreateEncoderResult::Err("Audio file recording requires iOS or Android.");
#endif
}

/// Points an open encoder at a new input format while it keeps writing the same file.
inline OpenEncoderResult reprepareOsEncoderInput(
    AudioEncoder &encoder,
    const StreamFormat &inputFormat) {
#if defined(__APPLE__) && RN_AUDIO_API_HAS_OS_ENCODER
  return static_cast<os_encoder::Encoder &>(encoder).reprepareInput(inputFormat);
#else
  (void)encoder;
  (void)inputFormat;
  return OpenEncoderResult::Err("Changing the input format of an open file is iOS only");
#endif
}

} // namespace audioapi
