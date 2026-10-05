#pragma once

#include <audioapi/encoding/EncoderOutputSpec.h>

#include <array>
#include <optional>
#include <string_view>
#include <utility>

namespace audioapi::android::media_format {

/// True for an AMEDIAFORMAT_KEY_MIME value that names an audio track, e.g. "audio/mp4a-latm".
[[nodiscard]] inline bool isAudioMime(std::string_view mime) {
  return mime.starts_with("audio/");
}

inline constexpr std::array<std::pair<AudioCodec, std::string_view>, 4> CODEC_MIMES{{
    {AudioCodec::AAC, "audio/mp4a-latm"},
    {AudioCodec::FLAC, "audio/flac"},
    {AudioCodec::OPUS, "audio/opus"},
    {AudioCodec::VORBIS, "audio/vorbis"},
}};

/// The AMEDIAFORMAT_KEY_MIME value for @p codec, or nullopt for a codec outside CODEC_MIMES.
[[nodiscard]] inline std::optional<std::string_view> mimeForCodec(AudioCodec codec) {
  for (const auto &[knownCodec, mime] : CODEC_MIMES) {
    if (knownCodec == codec) {
      return mime;
    }
  }
  return std::nullopt;
}

/// The codec an AMEDIAFORMAT_KEY_MIME value names, or nullopt for one outside CODEC_MIMES.
[[nodiscard]] inline std::optional<AudioCodec> codecForMime(std::string_view mime) {
  for (const auto &[codec, knownMime] : CODEC_MIMES) {
    if (mime == knownMime) {
      return codec;
    }
  }
  return std::nullopt;
}

} // namespace audioapi::android::media_format
