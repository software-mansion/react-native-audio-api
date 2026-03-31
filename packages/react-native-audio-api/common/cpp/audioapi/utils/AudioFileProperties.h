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

  enum class Format {
    WAV = 0,
    CAF = 1,
    M4A = 2,
    FLAC = 3,
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
      size_t batchDurationSeconds,
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
  size_t batchDurationSeconds;
  Format format;
  float sampleRate;
  size_t bitRate;
  BitDepth bitDepth;
  int flacCompressionLevel;
  int androidFlushIntervalMs;
  IOSAudioQuality iosAudioQuality;
};

} // namespace audioapi
