#pragma once

#include <audioapi/decoding/backends/AudioDecoderBackend.h>
#include <audioapi/encoding/AudioEncoder.h>
#include <audioapi/utils/AudioFileProperties.h>
#include <audioapi/utils/Result.hpp>

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace audioapi {

using AudioFileConcatResult = Result<std::string, std::string>;

/// Builds the encoder that writes WAV and FLAC output. An empty function means the platform
/// has no encoder, and those formats fail with an "unavailable" error.
using ConcatEncoderFactory = std::function<std::unique_ptr<AudioEncoder>(const EncoderSettings &)>;

class AudioFileReader {
 public:
  AudioFileReader() = default;
  AudioFileReader(const AudioFileReader &) = delete;
  AudioFileReader &operator=(const AudioFileReader &) = delete;

  AudioFileReader(AudioFileReader &&other) noexcept;
  AudioFileReader &operator=(AudioFileReader &&other) noexcept;

  ~AudioFileReader();

  [[nodiscard]] static Result<AudioFileReader, std::string> open(const std::string &filePath);

  /// Interleaved float32.
  [[nodiscard]] size_t readPcmFrames(float *frames, size_t frameCount);

  [[nodiscard]] const std::string &filePath() const;

  [[nodiscard]] uint32_t sampleRate() const;

  [[nodiscard]] uint32_t channels() const;

  [[nodiscard]] uint64_t totalPcmFrames() const;

 private:
  void close();

  std::string filePath_;
  std::unique_ptr<decoding::AudioDecoderBackend> decoder_;
};

/// Concatenates compatible local audio files into a single output file.
[[nodiscard]] AudioFileConcatResult concatAudioFiles(
    const std::vector<std::string> &inputPaths,
    const std::string &outputPath);

/// WAV and FLAC output go through @p createEncoder instead of the platform encoder, so the
/// concat can run where no platform encoder exists.
[[nodiscard]] AudioFileConcatResult concatAudioFiles(
    const std::vector<std::string> &inputPaths,
    const std::string &outputPath,
    const ConcatEncoderFactory &createEncoder);

} // namespace audioapi
