#pragma once

#include <cstddef>
#include <memory>
#include <string>

namespace facebook {
namespace jsi {
class Runtime;
class Value;
} // namespace jsi
} // namespace facebook

namespace audioapi {

class AudioFileProperties {
 public:
  enum class FileDirectory {
    Document = 0,
    Cache = 1,
  };

  // Values must stay in sync with the TypeScript `FileFormat` enum in src/types.ts.
  // Each value maps to a concrete container+codec via EncoderCapabilities.
  // Availability is platform-dependent (see EncoderCapabilities::isSupported).
  enum class Format {
    WAV = 0,
    CAF = 1,
    M4A = 2,
    FLAC = 3,
    AIFF = 4,
    ALAC = 5,
    OPUS_OGG = 6,
    OPUS_WEBM = 7,
    VORBIS_WEBM = 8,
    AMR_NB = 9,
    AMR_WB = 10,
    AAC_HE = 11,
    AAC_ELD = 12,
    IMA4 = 13,
    ULAW = 14,
    ALAW = 15,
    ILBC = 16,
  };

  enum class IOSAudioQuality {
    Min = 0,
    Low = 1,
    Medium = 2,
    High = 3,
    Max = 4,
  };

  enum class BitDepth {
    Bit16 = 0,
    Bit24 = 1,
    Bit32 = 2,
  };

  AudioFileProperties(
      FileDirectory directory,
      const std::string &subDirectory,
      const std::string &fileNamePrefix,
      int channelCount,
      size_t rotateIntervalBytes,
      Format format,
      float sampleRate,
      size_t bitRate,
      BitDepth bitDepth,
      int flacCompressionLevel,
      int androidFlushIntervalMs,
      IOSAudioQuality iosAudioQuality);

  static std::shared_ptr<AudioFileProperties> CreateFromJSIValue(
      facebook::jsi::Runtime &runtime,
      const facebook::jsi::Value &value);

  FileDirectory directory;
  std::string subDirectory;
  std::string fileNamePrefix;
  int channelCount;
  size_t rotateIntervalBytes;
  Format format;
  float sampleRate;
  size_t bitRate;
  BitDepth bitDepth;
  int flacCompressionLevel;
  int androidFlushIntervalMs;
  IOSAudioQuality iosAudioQuality;
};

} // namespace audioapi
