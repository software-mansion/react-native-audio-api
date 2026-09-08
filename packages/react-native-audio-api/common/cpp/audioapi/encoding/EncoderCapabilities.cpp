#include <audioapi/encoding/EncoderCapabilities.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <string>

namespace audioapi::EncoderCapabilities {

using Format = AudioFileProperties::Format;

namespace {

constexpr size_t kFormatCount = static_cast<size_t>(Format::ALAW) + 1;

constexpr std::array kSpecsByFormat = {
    EncoderOutputSpec{
        .container = AudioContainer::WAV,
        .codec = AudioCodec::PCM,
        .extension = "wav"}, // WAV
    EncoderOutputSpec{
        .container = AudioContainer::CAF,
        .codec = AudioCodec::PCM,
        .extension = "caf"}, // CAF
    EncoderOutputSpec{
        .container = AudioContainer::M4A,
        .codec = AudioCodec::AAC,
        .extension = "m4a"}, // M4A
    EncoderOutputSpec{
        .container = AudioContainer::FLAC,
        .codec = AudioCodec::FLAC,
        .extension = "flac"}, // FLAC
    EncoderOutputSpec{
        .container = AudioContainer::AIFF,
        .codec = AudioCodec::PCM,
        .extension = "aiff"}, // AIFF
    EncoderOutputSpec{
        .container = AudioContainer::M4A,
        .codec = AudioCodec::ALAC,
        .extension = "m4a"}, // ALAC
    EncoderOutputSpec{
        .container = AudioContainer::OGG,
        .codec = AudioCodec::OPUS,
        .extension = "ogg"}, // OPUS_OGG
    EncoderOutputSpec{
        .container = AudioContainer::WEBM,
        .codec = AudioCodec::OPUS,
        .extension = "webm"}, // OPUS_WEBM
    EncoderOutputSpec{
        .container = AudioContainer::WEBM,
        .codec = AudioCodec::VORBIS,
        .extension = "webm"}, // VORBIS_WEBM
    EncoderOutputSpec{
        .container = AudioContainer::WAV,
        .codec = AudioCodec::ULAW,
        .extension = "wav"}, // ULAW
    EncoderOutputSpec{
        .container = AudioContainer::WAV,
        .codec = AudioCodec::ALAW,
        .extension = "wav"}, // ALAW
};

static_assert(
    std::tuple_size_v<decltype(kSpecsByFormat)> == kFormatCount,
    "Every AudioFileProperties::Format needs an entry in kSpecsByFormat");

} // namespace

EncoderOutputSpec specForFormat(Format format) {
  const auto index = static_cast<size_t>(format);
  return index < kSpecsByFormat.size() ? kSpecsByFormat[index]
                                       : kSpecsByFormat[static_cast<size_t>(Format::WAV)];
}

bool isSupported(AudioContainer container, AudioCodec codec) {
  return std::ranges::any_of(
      kSupportedOutputSpecs, [container, codec](const EncoderOutputSpec &spec) {
        return spec.container == container && spec.codec == codec;
      });
}

Result<EncoderOutputSpec, std::string> resolveOutputSpec(Format format) {
  EncoderOutputSpec spec = specForFormat(format);
  if (isSupported(spec.container, spec.codec)) {
    return Result<EncoderOutputSpec, std::string>::Ok(spec);
  }

  std::string message = std::string(toString(spec.codec)) + " in " + toString(spec.container) +
      " is not encodable via system APIs on this platform. Choose a format supported by the "
      "device's system encoders.";
  return Result<EncoderOutputSpec, std::string>::Err(message);
}

} // namespace audioapi::EncoderCapabilities
