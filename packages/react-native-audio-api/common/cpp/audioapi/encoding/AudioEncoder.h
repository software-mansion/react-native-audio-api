#pragma once

#include <audioapi/encoding/EncoderOutputSpec.h>
#include <audioapi/encoding/StreamFormat.h>
#include <audioapi/utils/AudioFileProperties.h>
#include <audioapi/utils/AudioLayout.h>
#include <audioapi/utils/Macros.h>
#include <audioapi/utils/Result.hpp>

#include <atomic>
#include <cstddef>
#include <string>
#include <tuple>

namespace audioapi {

using OpenEncoderResult = Result<std::string, std::string>;
using EncodeResult = Result<size_t, std::string>;
using CloseEncoderResult = Result<std::tuple<double, double>, std::string>;

/// What an encoder writes. The rest of AudioFileProperties (where the file goes, how a
/// recording rotates) is the writer's business, so encoders never see it.
struct EncoderSettings {
  /// Sample rate and channel count of the file; the encoder converts its input to them.
  AudioLayout stream;
  AudioFileProperties::EncodingConfig encoding;
};

/// Incremental audio encoder. `open`/`close` on the JS thread; `encode` on the
/// file-writer worker. Platform implementations use system APIs only.
class AudioEncoder {
 public:
  explicit AudioEncoder(const EncoderSettings &settings) : settings_(settings) {}
  virtual ~AudioEncoder() = default;
  DELETE_COPY_AND_MOVE(AudioEncoder);

  virtual OpenEncoderResult open(
      const StreamFormat &inputFormat,
      const EncoderOutputSpec &outputSpec,
      const std::string &filePath) = 0;

  /// @p channels holds inputFormat.channelCount pointers, each to numFrames float32 samples,
  /// valid only for the duration of the call.
  virtual EncodeResult encode(const float *const *channels, int numFrames) = 0;

  /// Flushes and closes the output file. Returns {sizeMB, durationSeconds}.
  virtual CloseEncoderResult close() = 0;

  [[nodiscard]] bool isOpen() const {
    return isOpen_.load(std::memory_order_acquire);
  }
  [[nodiscard]] const std::string &getFilePath() const {
    return filePath_;
  }
  [[nodiscard]] virtual size_t getFileSizeBytes() const = 0;

 protected:
  void markOpen() {
    isOpen_.store(true, std::memory_order_release);
  }
  void markClosed() {
    isOpen_.store(false, std::memory_order_release);
  }

  [[nodiscard]] double getEncodedDurationSeconds() const {
    const double sampleRate = static_cast<double>(settings_.stream.sampleRate);
    if (sampleRate <= 0.0) {
      return 0.0;
    }
    return static_cast<double>(framesEncoded_.load(std::memory_order_acquire)) / sampleRate;
  }

  void resetFramesEncoded() {
    framesEncoded_.store(0, std::memory_order_release);
  }

  void addEncodedFrames(size_t frames) {
    framesEncoded_.fetch_add(frames, std::memory_order_acq_rel);
  }

  EncoderSettings settings_;
  StreamFormat inputFormat_;
  EncoderOutputSpec outputSpec_;
  std::string filePath_;
  std::atomic<size_t> framesEncoded_{0};

 private:
  std::atomic<bool> isOpen_{false};
};

} // namespace audioapi
