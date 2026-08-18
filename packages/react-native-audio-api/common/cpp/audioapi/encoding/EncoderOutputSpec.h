#pragma once

#include <cstdint>
#include <string>

namespace audioapi {

enum class AudioCodec : uint8_t {
  PCM,
  AAC,
  ALAC,
  FLAC,
  ULAW,
  ALAW,
  OPUS,
  VORBIS,
};

enum class AudioContainer : uint8_t {
  WAV,
  CAF,
  AIFF,
  M4A,
  WEBM,
  OGG,
  FLAC,
};

struct EncoderOutputSpec {
  AudioContainer container = AudioContainer::WAV;
  AudioCodec codec = AudioCodec::PCM;
  std::string extension = "wav";
};

inline const char *toString(AudioCodec codec) {
  switch (codec) {
    case AudioCodec::PCM:
      return "PCM";
    case AudioCodec::AAC:
      return "AAC";
    case AudioCodec::ALAC:
      return "ALAC";
    case AudioCodec::FLAC:
      return "FLAC";
    case AudioCodec::ULAW:
      return "u-law";
    case AudioCodec::ALAW:
      return "a-law";
    case AudioCodec::OPUS:
      return "Opus";
    case AudioCodec::VORBIS:
      return "Vorbis";
  }
  return "unknown";
}

inline const char *toString(AudioContainer container) {
  switch (container) {
    case AudioContainer::WAV:
      return "WAV";
    case AudioContainer::CAF:
      return "CAF";
    case AudioContainer::AIFF:
      return "AIFF";
    case AudioContainer::M4A:
      return "M4A";
    case AudioContainer::WEBM:
      return "WebM";
    case AudioContainer::OGG:
      return "OGG";
    case AudioContainer::FLAC:
      return "FLAC";
  }
  return "unknown";
}

} // namespace audioapi
