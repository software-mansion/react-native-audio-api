#include <audioapi/encoding/EncoderCapabilities.h>

#include <algorithm>
#include <string>
#include <vector>

namespace audioapi {

using Format = AudioFileProperties::Format;

EncoderOutputSpec EncoderCapabilities::specForFormat(Format format) {
  switch (format) {
    case Format::WAV:
      return {.container = AudioContainer::WAV, .codec = AudioCodec::PCM, .extension = "wav"};
    case Format::CAF:
      return {.container = AudioContainer::CAF, .codec = AudioCodec::PCM, .extension = "caf"};
    case Format::M4A:
      return {.container = AudioContainer::M4A, .codec = AudioCodec::AAC, .extension = "m4a"};
    case Format::FLAC:
      return {.container = AudioContainer::FLAC, .codec = AudioCodec::FLAC, .extension = "flac"};
    case Format::AIFF:
      return {.container = AudioContainer::AIFF, .codec = AudioCodec::PCM, .extension = "aiff"};
    case Format::ALAC:
      return {.container = AudioContainer::M4A, .codec = AudioCodec::ALAC, .extension = "m4a"};
    case Format::OPUS_OGG:
      return {.container = AudioContainer::OGG, .codec = AudioCodec::OPUS, .extension = "ogg"};
    case Format::OPUS_WEBM:
      return {.container = AudioContainer::WEBM, .codec = AudioCodec::OPUS, .extension = "webm"};
    case Format::VORBIS_WEBM:
      return {.container = AudioContainer::WEBM, .codec = AudioCodec::VORBIS, .extension = "webm"};
    case Format::AMR_NB:
      return {
          .container = AudioContainer::THREE_GP, .codec = AudioCodec::AMR_NB, .extension = "3gp"};
    case Format::AMR_WB:
      return {
          .container = AudioContainer::THREE_GP, .codec = AudioCodec::AMR_WB, .extension = "3gp"};
    case Format::AAC_HE:
      return {.container = AudioContainer::M4A, .codec = AudioCodec::AAC_HE, .extension = "m4a"};
    case Format::AAC_ELD:
      return {.container = AudioContainer::M4A, .codec = AudioCodec::AAC_ELD, .extension = "m4a"};
    case Format::IMA4:
      return {.container = AudioContainer::CAF, .codec = AudioCodec::IMA4, .extension = "caf"};
    case Format::ULAW:
      return {.container = AudioContainer::WAV, .codec = AudioCodec::ULAW, .extension = "wav"};
    case Format::ALAW:
      return {.container = AudioContainer::WAV, .codec = AudioCodec::ALAW, .extension = "wav"};
    case Format::ILBC:
      return {.container = AudioContainer::CAF, .codec = AudioCodec::ILBC, .extension = "caf"};
  }
  return {.container = AudioContainer::WAV, .codec = AudioCodec::PCM, .extension = "wav"};
}

std::vector<EncoderOutputSpec> EncoderCapabilities::probe() {
#ifdef __APPLE__
  return {
      {.container = AudioContainer::WAV, .codec = AudioCodec::PCM, .extension = "wav"},
      {.container = AudioContainer::CAF, .codec = AudioCodec::PCM, .extension = "caf"},
      {.container = AudioContainer::AIFF, .codec = AudioCodec::PCM, .extension = "aiff"},
      {.container = AudioContainer::M4A, .codec = AudioCodec::AAC, .extension = "m4a"},
      {.container = AudioContainer::M4A, .codec = AudioCodec::AAC_HE, .extension = "m4a"},
      {.container = AudioContainer::M4A, .codec = AudioCodec::AAC_ELD, .extension = "m4a"},
      {.container = AudioContainer::M4A, .codec = AudioCodec::ALAC, .extension = "m4a"},
      {.container = AudioContainer::FLAC, .codec = AudioCodec::FLAC, .extension = "flac"},
      {.container = AudioContainer::CAF, .codec = AudioCodec::IMA4, .extension = "caf"},
      {.container = AudioContainer::WAV, .codec = AudioCodec::ULAW, .extension = "wav"},
      {.container = AudioContainer::WAV, .codec = AudioCodec::ALAW, .extension = "wav"},
      {.container = AudioContainer::CAF, .codec = AudioCodec::ILBC, .extension = "caf"},
  };
#elif defined(__ANDROID__)
  return {
      {.container = AudioContainer::WAV, .codec = AudioCodec::PCM, .extension = "wav"},
      {.container = AudioContainer::M4A, .codec = AudioCodec::AAC, .extension = "m4a"},
      {.container = AudioContainer::FLAC, .codec = AudioCodec::FLAC, .extension = "flac"},
      {.container = AudioContainer::OGG, .codec = AudioCodec::OPUS, .extension = "ogg"},
      {.container = AudioContainer::WEBM, .codec = AudioCodec::OPUS, .extension = "webm"},
      {.container = AudioContainer::WEBM, .codec = AudioCodec::VORBIS, .extension = "webm"},
      {.container = AudioContainer::THREE_GP, .codec = AudioCodec::AMR_NB, .extension = "3gp"},
      {.container = AudioContainer::THREE_GP, .codec = AudioCodec::AMR_WB, .extension = "3gp"},
  };
#else
  return {};
#endif
}

bool EncoderCapabilities::isSupported(AudioContainer container, AudioCodec codec) {
  return std::ranges::any_of(probe(), [container, codec](const EncoderOutputSpec &spec) {
    return spec.container == container && spec.codec == codec;
  });
}

Result<EncoderOutputSpec, std::string> EncoderCapabilities::resolveOutputSpec(Format format) {
  EncoderOutputSpec spec = specForFormat(format);
  if (isSupported(spec.container, spec.codec)) {
    return Result<EncoderOutputSpec, std::string>::Ok(spec);
  }

  std::string message = std::string(toString(spec.codec)) + " in " + toString(spec.container) +
      " is not encodable via system APIs on this platform. Choose a format supported by the "
      "device's system encoders.";
  return Result<EncoderOutputSpec, std::string>::Err(message);
}

} // namespace audioapi
