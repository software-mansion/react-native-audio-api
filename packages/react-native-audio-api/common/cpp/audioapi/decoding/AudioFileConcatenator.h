#pragma once

#include <audioapi/decoding/backends/AudioDecoderBackend.h>
#include <audioapi/libs/miniaudio/miniaudio.h>
#include <audioapi/utils/Result.hpp>

#include <memory>
#include <string>
#include <vector>

#if !RN_AUDIO_API_FFMPEG_DISABLED
struct AVDictionary;
struct AVFormatContext;
struct AVStream;
#endif // RN_AUDIO_API_FFMPEG_DISABLED

namespace audioapi {

using AudioFileConcatResult = Result<std::string, std::string>;

class AudioDecoderBackendGuard {
 public:
  AudioDecoderBackendGuard() = default;
  AudioDecoderBackendGuard(const AudioDecoderBackendGuard &) = delete;
  AudioDecoderBackendGuard &operator=(const AudioDecoderBackendGuard &) = delete;

  AudioDecoderBackendGuard(AudioDecoderBackendGuard &&other) noexcept;
  AudioDecoderBackendGuard &operator=(AudioDecoderBackendGuard &&other) noexcept;

  ~AudioDecoderBackendGuard();

  [[nodiscard]] static Result<AudioDecoderBackendGuard, std::string> open(
      const std::string &filePath);

  [[nodiscard]] size_t readPcmFrames(float *frames, size_t frameCount);

  [[nodiscard]] const std::string &filePath() const;

  [[nodiscard]] ma_uint32 sampleRate() const;

  [[nodiscard]] ma_uint32 channels() const;

  [[nodiscard]] ma_format format() const {
    return ma_format_f32;
  }

  [[nodiscard]] ma_uint64 totalPcmFrames() const;

 private:
  void close();

  std::string filePath_;
  std::unique_ptr<decoding::AudioDecoderBackend> decoder_;
};

class MiniAudioEncoderGuard {
 public:
  MiniAudioEncoderGuard() = default;
  MiniAudioEncoderGuard(const MiniAudioEncoderGuard &) = delete;
  MiniAudioEncoderGuard &operator=(const MiniAudioEncoderGuard &) = delete;

  // Closes the owned miniaudio encoder.
  ~MiniAudioEncoderGuard();

  // Opens a WAV output file with the provided PCM parameters.
  [[nodiscard]] AudioFileConcatResult
  open(const std::string &outputPath, ma_format format, ma_uint32 sampleRate, ma_uint32 channels);

  // Writes decoded PCM frames into the WAV output file.
  [[nodiscard]] AudioFileConcatResult
  write(const std::string &inputPath, const void *frames, ma_uint64 frameCount);

 private:
  void close();

  ma_encoder encoder_{};
  bool initialized_{false};
};

#if !RN_AUDIO_API_FFMPEG_DISABLED
class AVDictionaryGuard {
 public:
  AVDictionaryGuard() = default;
  AVDictionaryGuard(const AVDictionaryGuard &) = delete;
  AVDictionaryGuard &operator=(const AVDictionaryGuard &) = delete;

  // Releases the owned FFmpeg dictionary.
  ~AVDictionaryGuard();

  // Returns the dictionary pointer address expected by FFmpeg option APIs.
  [[nodiscard]] AVDictionary **ptr();

 private:
  AVDictionary *dictionary_{nullptr};
};

class InputFormatContext {
 public:
  InputFormatContext() = default;

  // Takes ownership of an opened FFmpeg input context and selected audio stream.
  InputFormatContext(std::string filePath, AVFormatContext *context, int audioStreamIndex);

  InputFormatContext(const InputFormatContext &) = delete;
  InputFormatContext &operator=(const InputFormatContext &) = delete;

  // Moves ownership without closing the transferred FFmpeg context.
  InputFormatContext(InputFormatContext &&other) noexcept;
  InputFormatContext &operator=(InputFormatContext &&other) noexcept;

  // Closes the owned FFmpeg input context.
  ~InputFormatContext();

  // Returns the owned FFmpeg input context.
  [[nodiscard]] AVFormatContext *context() const;

  // Returns the selected audio stream from the input context.
  [[nodiscard]] AVStream *audioStream() const;

  // Returns the selected audio stream index from the input context.
  [[nodiscard]] int audioStreamIndex() const;

  // Returns the normalized file path represented by this input.
  [[nodiscard]] const std::string &filePath() const;

 private:
  void close();

  std::string filePath_;
  AVFormatContext *context_{nullptr};
  int audioStreamIndex_{-1};
};

class OutputFormatContext {
 public:
  // Takes ownership of an allocated FFmpeg output context.
  explicit OutputFormatContext(AVFormatContext *context);
  OutputFormatContext(const OutputFormatContext &) = delete;
  OutputFormatContext &operator=(const OutputFormatContext &) = delete;

  // Closes the output file handle and frees the owned FFmpeg output context.
  ~OutputFormatContext();

  // Returns the owned FFmpeg output context.
  [[nodiscard]] AVFormatContext *get() const;

 private:
  AVFormatContext *context_{nullptr};
};
#endif // RN_AUDIO_API_FFMPEG_DISABLED

// Concatenates compatible local audio files into a single output file.
[[nodiscard]] AudioFileConcatResult concatAudioFiles(
    const std::vector<std::string> &inputPaths,
    const std::string &outputPath);

// Converts a local path or file:// URL into a decoded filesystem path.
[[nodiscard]] std::string normalizeFilePath(const std::string &path);

} // namespace audioapi
