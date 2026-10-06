#pragma once

#include <audioapi/encoding/EncoderOutputSpec.h>
#include <audioapi/utils/AudioFileProperties.h>
#include <audioapi/utils/Result.hpp>

#include <array>
#include <string>

/// Declared system-API encoding capabilities of the current platform.
/// A device may still reject a format at `AudioEncoder::open()`.
namespace audioapi::encoder_capabilities {

/// Container/codec/extension of every file format, indexed by `AudioFileProperties::FileFormat`.
inline constexpr std::array kSpecsByFormat = {
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
        .container = AudioContainer::ADTS,
        .codec = AudioCodec::AAC,
        .extension = "aac"}, // ADTS
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
    kSpecsByFormat.size() == static_cast<size_t>(AudioFileProperties::FileFormat::ALAW) + 1,
    "Every AudioFileProperties::FileFormat needs an entry in kSpecsByFormat");

/// Formats the platform's system encoders can produce.
#ifdef __APPLE__
inline constexpr std::array kSupportedFormats = {
    AudioFileProperties::FileFormat::WAV,
    AudioFileProperties::FileFormat::CAF,
    AudioFileProperties::FileFormat::AIFF,
    AudioFileProperties::FileFormat::M4A,
    AudioFileProperties::FileFormat::ALAC,
    AudioFileProperties::FileFormat::FLAC,
    AudioFileProperties::FileFormat::ULAW,
    AudioFileProperties::FileFormat::ALAW,
    AudioFileProperties::FileFormat::ADTS,
};
#elif defined(__ANDROID__)
inline constexpr std::array kSupportedFormats = {
    AudioFileProperties::FileFormat::WAV,
    AudioFileProperties::FileFormat::M4A,
    AudioFileProperties::FileFormat::FLAC,
    AudioFileProperties::FileFormat::OPUS_OGG,
    AudioFileProperties::FileFormat::OPUS_WEBM,
    AudioFileProperties::FileFormat::VORBIS_WEBM,
    AudioFileProperties::FileFormat::ADTS,
};
#else
inline constexpr std::array<AudioFileProperties::FileFormat, 0> kSupportedFormats = {};
#endif

bool isSupported(AudioContainer container, AudioCodec codec);

/// Container/codec/extension mapping for a file format (ignores platform support).
EncoderOutputSpec specForFormat(AudioFileProperties::FileFormat format);

/// Maps a format to a supported output spec, or an error if unavailable here.
Result<EncoderOutputSpec, std::string> resolveOutputSpec(AudioFileProperties::FileFormat format);

} // namespace audioapi::encoder_capabilities
