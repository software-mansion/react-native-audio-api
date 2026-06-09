#pragma once

#include <audioapi/utils/Macros.h>
#include <audioapi/utils/Result.hpp>
#include <cstddef>
#include <string>

namespace audioapi::decoding {
using DecoderResult = Result<NoneType, std::string>;
/**
 * Incremental PCM decoder: openFile or openMemory → readPcmFrames in a loop → close.
 * Shared contract for FFmpeg-based and MiniAudio-based implementations.
 */
class IncrementalAudioDecoder {
 public:
  static constexpr size_t CHUNK_SIZE = 4096;
  IncrementalAudioDecoder() = default;
  virtual ~IncrementalAudioDecoder() = default;
  DELETE_COPY_AND_MOVE(IncrementalAudioDecoder);

  /// @brief Opens a file for decoding.
  /// @param outputSampleRate The output sample rate.
  /// @param path The path to the file.
  /// @return Ok(None) on success or Err(message) on failure.
  [[nodiscard]] virtual DecoderResult openFile(int outputSampleRate, const std::string &path) = 0;

  /// @brief Opens a memory block for decoding.
  /// @param outputSampleRate The output sample rate.
  /// @param data The data to decode.
  /// @param size The size of the data.
  /// @return Ok(None) on success or Err(message) on failure.
  [[nodiscard]] virtual DecoderResult
  openMemory(int outputSampleRate, const void *data, size_t size) = 0;

  /// @brief Reads PCM frames from the decoder.
  /// @param outInterleaved The output buffer for the decoded frames.
  /// @param frameCount The number of frames to read.
  /// @return The number of frames read.
  [[nodiscard]] virtual size_t readPcmFrames(float *outInterleaved, size_t frameCount) = 0;

  /// @brief Closes the decoder.
  virtual void close() = 0;

  /// @brief Checks if the decoder is open.
  /// @return True if the decoder is open, false otherwise.
  [[nodiscard]] virtual bool isOpen() const = 0;

  /// @brief Gets the number of output channels.
  /// @return The number of output channels.
  [[nodiscard]] virtual int outputChannels() const = 0;

  /// @brief Gets the output sample rate.
  /// @return The output sample rate.
  [[nodiscard]] virtual int outputSampleRate() const = 0;

  /// @brief True when the source uses FFmpeg's live HLS demuxer (indefinite stream).
  [[nodiscard]] virtual bool isHlsStreaming() const {
    return false;
  }

  /// @brief Gets the duration of the audio in seconds.
  /// @return The duration of the audio in seconds.
  [[nodiscard]] virtual float getDurationInSeconds() const = 0;

  /// @brief Gets the current position of the audio in seconds.
  /// @return The current position of the audio in seconds.
  [[nodiscard]] virtual float getCurrentPositionInSeconds() const = 0;

  /// @brief Seeks to a specific time in the audio.
  /// @param seconds The time to seek to in seconds.
  /// @return Ok(None) on success or Err(message) on failure.
  [[nodiscard]] virtual DecoderResult seekToTime(double seconds) = 0;
};
} // namespace audioapi::decoding
