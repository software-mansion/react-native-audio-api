#pragma once

#include <audioapi/utils/Macros.h>
#include <audioapi/utils/Result.hpp>
#include <cstddef>
#include <cstdint>
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

  [[nodiscard]] virtual int outputChannels() const {
    return outputChannels_;
  }
  [[nodiscard]] virtual int outputSampleRate() const {
    return outputSampleRate_;
  }
  [[nodiscard]] virtual float getDurationInSeconds() const {
    if (outputSampleRate_ <= 0 || totalPcmFrames_ == 0) {
      return 0.0f;
    }
    return static_cast<float>(
        static_cast<double>(totalPcmFrames_) / static_cast<double>(outputSampleRate_));
  }
  [[nodiscard]] virtual float getCurrentPositionInSeconds() const {
    if (outputSampleRate_ <= 0) {
      return 0.0f;
    }
    return static_cast<float>(
        static_cast<double>(framePosition_) / static_cast<double>(outputSampleRate_));
  }
  [[nodiscard]] virtual bool isHlsStreaming() const {
    return false;
  }
  /// Total PCM frame count when known; 0 when unavailable.
  [[nodiscard]] virtual size_t getTotalPcmFrameCount() const {
    return totalPcmFrames_;
  }

 protected:
  void resetOpenMetadata() {
    outputChannels_ = 0;
    outputSampleRate_ = 0;
    framePosition_ = 0;
    totalPcmFrames_ = 0;
  }

  /// Cache length from a duration in seconds (no-op when rate/duration invalid).
  void setTotalPcmFramesFromDuration(double durationSeconds) {
    if (durationSeconds > 0.0 && outputSampleRate_ > 0) {
      totalPcmFrames_ =
          static_cast<size_t>(durationSeconds * static_cast<double>(outputSampleRate_));
    } else {
      totalPcmFrames_ = 0;
    }
  }

  int outputChannels_ = 0;
  int outputSampleRate_ = 0;
  int64_t framePosition_ = 0;
  size_t totalPcmFrames_ = 0;
};

} // namespace audioapi::decoding
