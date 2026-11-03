#include <audioapi/encoding/EncoderFactory.h>
#include <audioapi/encoding/OSEncoding.h>
#include <audioapi/utils/AudioFileProperties.h>

#if !RN_AUDIO_API_STATIC_EXTERNAL_LIBS_DISABLED
#include <audioapi/encoding/OggOpusEncoder.h>
#endif

#include <memory>

namespace audioapi::encoding {

CreateEncoderResult createEncoder(const EncoderSettings &settings) {
#if !RN_AUDIO_API_STATIC_EXTERNAL_LIBS_DISABLED
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

} // namespace audioapi::encoding
