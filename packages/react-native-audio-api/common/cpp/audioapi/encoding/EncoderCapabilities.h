#pragma once

#include <audioapi/encoding/EncoderOutputSpec.h>
#include <audioapi/utils/AudioFileProperties.h>
#include <audioapi/utils/Result.hpp>

#include <array>
#include <string>

/// Declared system-API encoding capabilities of the current platform.
/// A device may still reject a format at `AudioEncoder::open()`.
namespace audioapi::EncoderCapabilities {

/// Container/codec/extension of every file format, indexed by `AudioFileProperties::Format`.
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
    kSpecsByFormat.size() == static_cast<size_t>(AudioFileProperties::Format::ALAW) + 1,
    "Every AudioFileProperties::Format needs an entry in kSpecsByFormat");

/// Formats the platform's system encoders can produce.
#ifdef __APPLE__
inline constexpr std::array kSupportedFormats = {
    AudioFileProperties::Format::WAV,
    AudioFileProperties::Format::CAF,
    AudioFileProperties::Format::AIFF,
    AudioFileProperties::Format::M4A,
    AudioFileProperties::Format::ALAC,
    AudioFileProperties::Format::FLAC,
    AudioFileProperties::Format::ULAW,
    AudioFileProperties::Format::ALAW,
};
#elif defined(__ANDROID__)
inline constexpr std::array kSupportedFormats = {
    AudioFileProperties::Format::WAV,
    AudioFileProperties::Format::M4A,
    AudioFileProperties::Format::FLAC,
    AudioFileProperties::Format::OPUS_OGG,
    AudioFileProperties::Format::OPUS_WEBM,
    AudioFileProperties::Format::VORBIS_WEBM,
};
#else
inline constexpr std::array<AudioFileProperties::Format, 0> kSupportedFormats = {};
#endif

bool isSupported(AudioContainer container, AudioCodec codec);

/// Container/codec/extension mapping for a file format (ignores platform support).
EncoderOutputSpec specForFormat(AudioFileProperties::Format format);

/// Maps a format to a supported output spec, or an error if unavailable here.
Result<EncoderOutputSpec, std::string> resolveOutputSpec(AudioFileProperties::Format format);

} // namespace audioapi::EncoderCapabilities
