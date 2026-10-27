#pragma once

#include <audioapi/encoding/AudioEncoder.h>
#include <audioapi/encoding/OggOpusEncoder.h>
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

/// The encoder for the format in @p settings: the platform's system encoder, except Opus,
/// which has the shared encoder implementation.
inline CreateEncoderResult createPlatformEncoder(const EncoderSettings &settings) {
#if RN_AUDIO_API_BUNDLED_OGG_OPUS
  if (settings.encoding.format == AudioFileProperties::FileFormat::OPUS_OGG) {
    return CreateEncoderResult::Ok(std::make_unique<ogg_opus::OggOpusEncoder>(settings));
  }
#endif
#if RN_AUDIO_API_HAS_OS_ENCODER
  return CreateEncoderResult::Ok(std::make_unique<os_encoder::Encoder>(settings));
#else
  (void)settings;
  return CreateEncoderResult::Err("Audio file recording requires iOS or Android.");
#endif
}

} // namespace audioapi
