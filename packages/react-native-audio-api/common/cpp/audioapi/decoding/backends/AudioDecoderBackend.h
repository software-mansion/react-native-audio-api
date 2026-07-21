#pragma once

#include <audioapi/utils/Macros.h>
#include <audioapi/utils/Result.hpp>
#include <cstddef>
#include <string>

namespace audioapi::decoding {
using DecoderError = std::string;
using DecoderResult = Result<NoneType, DecoderError>;

class AudioDecoderBackend {
 public:
  static constexpr size_t CHUNK_SIZE = 4096;
  AudioDecoderBackend() = default;
  virtual ~AudioDecoderBackend() = default;
  DELETE_COPY_AND_MOVE(AudioDecoderBackend);

  [[nodiscard]] virtual size_t readPcmFrames(float *outInterleaved, size_t frameCount) = 0;
  [[nodiscard]] virtual DecoderResult seekToTime(double seconds) = 0;
  virtual void close() = 0;
  [[nodiscard]] virtual bool isOpen() const = 0;
  [[nodiscard]] virtual int outputChannels() const = 0;
  [[nodiscard]] virtual int outputSampleRate() const = 0;
  [[nodiscard]] virtual float getDurationInSeconds() const = 0;
  [[nodiscard]] virtual float getCurrentPositionInSeconds() const = 0;
  [[nodiscard]] virtual bool isHlsStreaming() const {
    return false;
  }
  /// Total PCM frame count when known (e.g. WAV); 0 when unavailable.
  [[nodiscard]] virtual size_t getTotalPcmFrameCount() const {
    return 0;
  }
};

} // namespace audioapi::decoding
