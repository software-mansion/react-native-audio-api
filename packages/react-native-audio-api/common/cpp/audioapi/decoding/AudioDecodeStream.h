#pragma once

#include <audioapi/decoding/AudioDecoding.h>
#include <audioapi/decoding/DecoderFactory.h>
#include <audioapi/utils/Macros.h>
#include <audioapi/utils/Result.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace audioapi::audiodecoding {

/// @brief A decoder kept open so its PCM can be pulled a bounded number of frames at a time,
/// instead of decoding the whole source into one buffer. Memory then follows the size of a
/// read, not the duration of the source.
/// @note Not thread-safe. `read`, `seek` and `close` must never run concurrently; the owner
/// serializes them. The format getters return values fixed at open and are safe from any thread.
class AudioDecodeStream {
 public:
  using OpenResult = Result<std::unique_ptr<AudioDecodeStream>, std::string>;
  using SeekResult = Result<NoneType, std::string>;

  /// @param sampleRate Output sample rate; 0 keeps the rate of the source.
  [[nodiscard]] static OpenResult openWithFilePath(const std::string &path, float sampleRate);
  /// @note Takes ownership of @p encodedBytes and keeps them for as long as the stream is open.
  [[nodiscard]] static OpenResult openWithMemoryBlock(
      std::vector<uint8_t> encodedBytes,
      float sampleRate);

  ~AudioDecodeStream();
  DELETE_COPY_AND_MOVE(AudioDecodeStream);

  /// @brief Decodes the next @p frameCount frames.
  /// @return A buffer of @p frameCount frames, shorter only when the stream ends inside it;
  /// `nullptr` once the stream has ended. An error after `close`.
  /// @note The decoder backends report a decoding failure as an end of stream, so a source
  /// that fails midway also ends with `nullptr`, as it does when decoded whole.
  [[nodiscard]] AudioBufferResult read(size_t frameCount);

  /// @brief Moves the read position. How close the decoder lands to @p seconds depends on the
  /// format: compressed formats seek to a frame boundary.
  [[nodiscard]] SeekResult seek(double seconds);

  /// @brief Releases the decoder. Idempotent.
  void close();

  [[nodiscard]] int numberOfChannels() const {
    return numberOfChannels_;
  }
  [[nodiscard]] float sampleRate() const {
    return sampleRate_;
  }
  /// @brief Duration reported by the container, in seconds; 0 when it is unknown.
  [[nodiscard]] float duration() const {
    return duration_;
  }
  /// @brief Bytes of encoded audio the stream keeps in memory; 0 for a file.
  [[nodiscard]] size_t encodedSizeInBytes() const {
    return encodedSizeInBytes_;
  }

 private:
  AudioDecodeStream(decoding::DecoderPtr decoder, size_t encodedSizeInBytes);

  [[nodiscard]] static OpenResult
  open(const decoding::DecoderSource &source, size_t encodedSizeInBytes, const char *errorMessage);

  decoding::DecoderPtr decoder_;
  const int numberOfChannels_;
  const float sampleRate_;
  const float duration_;
  const size_t encodedSizeInBytes_;
};

} // namespace audioapi::audiodecoding
