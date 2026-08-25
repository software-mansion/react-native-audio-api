#pragma once

#include <cstddef>
#include <memory>
#include <string>

namespace facebook::jsi {
class Runtime;
class Value;
} // namespace facebook::jsi

namespace audioapi {

class AudioFileProperties {
 public:
  enum class FileDirectory : std::uint8_t {
    Document = 0,
    Cache = 1,
  };

  // Values must stay in sync with the TypeScript `FileFormat` enum in src/types.ts.
  // Each value maps to a concrete container+codec via EncoderCapabilities.
  // Availability is platform-dependent (see EncoderCapabilities::isSupported).
  enum class Format : std::uint8_t {
    WAV = 0,
    CAF = 1,
    M4A = 2,
    FLAC = 3,
    AIFF = 4,
    ALAC = 5,
    OPUS_OGG = 6,
    OPUS_WEBM = 7,
    VORBIS_WEBM = 8,
    ULAW = 9,
    ALAW = 10,
  };

  enum class IOSAudioQuality : std::uint8_t {
    Min = 0,
    Low = 1,
    Medium = 2,
    High = 3,
    Max = 4,
  };

  enum class BitDepth : std::uint8_t {
    Bit16 = 0,
    Bit24 = 1,
    Bit32 = 2,
  };

  AudioFileProperties(
      FileDirectory directory,
      std::string subDirectory,
      std::string fileNamePrefix,
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
