#pragma once

#include <audioapi/encoding/EncoderOutputSpec.h>
#include <audioapi/utils/AudioFileProperties.h>
#include <audioapi/utils/Result.hpp>

#include <array>
#include <string>

/// Declared system-API encoding capabilities of the current platform.
/// A device may still reject a format at `AudioEncoder::open()`.
namespace audioapi::EncoderCapabilities {

#ifdef __APPLE__
inline constexpr std::array kSupportedOutputSpecs = {
    EncoderOutputSpec{
        .container = AudioContainer::WAV,
        .codec = AudioCodec::PCM,
        .extension = "wav"},
    EncoderOutputSpec{
        .container = AudioContainer::CAF,
        .codec = AudioCodec::PCM,
        .extension = "caf"},
    EncoderOutputSpec{
        .container = AudioContainer::AIFF,
        .codec = AudioCodec::PCM,
        .extension = "aiff"},
    EncoderOutputSpec{
        .container = AudioContainer::M4A,
        .codec = AudioCodec::AAC,
        .extension = "m4a"},
    EncoderOutputSpec{
        .container = AudioContainer::M4A,
        .codec = AudioCodec::ALAC,
        .extension = "m4a"},
    EncoderOutputSpec{
        .container = AudioContainer::FLAC,
        .codec = AudioCodec::FLAC,
        .extension = "flac"},
    EncoderOutputSpec{
        .container = AudioContainer::WAV,
        .codec = AudioCodec::ULAW,
        .extension = "wav"},
    EncoderOutputSpec{
        .container = AudioContainer::WAV,
        .codec = AudioCodec::ALAW,
        .extension = "wav"},
};
#elif defined(__ANDROID__)
inline constexpr std::array kSupportedOutputSpecs = {
    EncoderOutputSpec{
        .container = AudioContainer::WAV,
        .codec = AudioCodec::PCM,
        .extension = "wav"},
    EncoderOutputSpec{
        .container = AudioContainer::M4A,
        .codec = AudioCodec::AAC,
        .extension = "m4a"},
    EncoderOutputSpec{
        .container = AudioContainer::FLAC,
        .codec = AudioCodec::FLAC,
        .extension = "flac"},
    EncoderOutputSpec{
        .container = AudioContainer::OGG,
        .codec = AudioCodec::OPUS,
        .extension = "ogg"},
    EncoderOutputSpec{
        .container = AudioContainer::WEBM,
        .codec = AudioCodec::OPUS,
        .extension = "webm"},
    EncoderOutputSpec{
        .container = AudioContainer::WEBM,
        .codec = AudioCodec::VORBIS,
        .extension = "webm"},
};
#else
inline constexpr std::array<EncoderOutputSpec, 0> kSupportedOutputSpecs = {};
#endif

bool isSupported(AudioContainer container, AudioCodec codec);

/// Container/codec/extension mapping for a file format (ignores platform support).
EncoderOutputSpec specForFormat(AudioFileProperties::Format format);

/// Maps a format to a supported output spec, or an error if unavailable here.
Result<EncoderOutputSpec, std::string> resolveOutputSpec(AudioFileProperties::Format format);

} // namespace audioapi::EncoderCapabilities
