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
  // Availability is platform-dependent (see encoder_capabilities::isSupported).
  enum class FileFormat : std::uint8_t {
    WAV,
    CAF,
    M4A,
    FLAC,
    AIFF,
    ALAC,
    OPUS_OGG,
    OPUS_WEBM,
    VORBIS_WEBM,
    ULAW,
    ALAW,
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

  struct PathConfig {
    FileDirectory directory;
    std::string subDirectory;
    std::string fileName;
  };

  struct StreamConfig {
    float sampleRate;
    int channelCount;
  };

  struct EncodingConfig {
    FileFormat format;
    size_t bitRate;
    BitDepth bitDepth;
    int flacCompressionLevel;
    IOSAudioQuality iosAudioQuality;
  };

  struct WriterConfig {
    size_t rotateIntervalBytes;
    int androidFlushIntervalMs;
  };

  AudioFileProperties(
      PathConfig path,
      StreamConfig stream,
      EncodingConfig encoding,
      WriterConfig writer);

  static std::shared_ptr<AudioFileProperties> CreateFromJSIValue(
      facebook::jsi::Runtime &runtime,
      const facebook::jsi::Value &value);

  PathConfig path;
  StreamConfig stream;
  EncodingConfig encoding;
  WriterConfig writer;
};

} // namespace audioapi
