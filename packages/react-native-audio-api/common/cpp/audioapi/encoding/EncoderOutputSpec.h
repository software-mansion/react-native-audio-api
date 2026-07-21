#pragma once

#include <string>

namespace audioapi {

enum class AudioCodec : uint8_t {
  PCM,
  AAC,
  AAC_HE,
  AAC_ELD,
  AAC_HE_V2,
  ALAC,
  FLAC,
  IMA4,
  ULAW,
  ALAW,
  ILBC,
  AMR_NB,
  AMR_WB,
  OPUS,
  VORBIS,
};

enum class AudioContainer : uint8_t {
  WAV,
  CAF,
  AIFF,
  M4A,
  MP4,
  THREE_GP,
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
    case AudioCodec::AAC_HE:
      return "AAC-HE";
    case AudioCodec::AAC_ELD:
      return "AAC-ELD";
    case AudioCodec::AAC_HE_V2:
      return "AAC-HEv2";
    case AudioCodec::ALAC:
      return "ALAC";
    case AudioCodec::FLAC:
      return "FLAC";
    case AudioCodec::IMA4:
      return "IMA4";
    case AudioCodec::ULAW:
      return "u-law";
    case AudioCodec::ALAW:
      return "a-law";
    case AudioCodec::ILBC:
      return "iLBC";
    case AudioCodec::AMR_NB:
      return "AMR-NB";
    case AudioCodec::AMR_WB:
      return "AMR-WB";
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
    case AudioContainer::MP4:
      return "MP4";
    case AudioContainer::THREE_GP:
      return "3GP";
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
