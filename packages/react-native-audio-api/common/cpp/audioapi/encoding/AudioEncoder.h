#pragma once

#include <audioapi/encoding/EncoderOutputSpec.h>
#include <audioapi/encoding/StreamFormat.h>
#include <audioapi/utils/AudioFileProperties.h>
#include <audioapi/utils/Macros.h>
#include <audioapi/utils/Result.hpp>

#include <atomic>
#include <cstddef>
#include <memory>
#include <string>
#include <tuple>

namespace audioapi {

using OpenEncoderResult = Result<std::string, std::string>;
using EncodeResult = Result<size_t, std::string>;
using CloseEncoderResult = Result<std::tuple<double, double>, std::string>;

/// Incremental audio encoder. `open`/`close` on the JS thread; `encode` on the
/// file-writer worker. Platform implementations use system APIs only.
class AudioEncoder {
 public:
  explicit AudioEncoder(const std::shared_ptr<AudioFileProperties> &fileProperties)
      : fileProperties_(fileProperties) {}
  virtual ~AudioEncoder() = default;
  DELETE_COPY_AND_MOVE(AudioEncoder);

  virtual OpenEncoderResult open(
      const StreamFormat &inputFormat,
      const EncoderOutputSpec &outputSpec,
      size_t maxBufferSizeInFrames,
      const std::string &filePath) = 0;

  /// @p data points to numFrames * inputFormat.channelCount float32 samples in
  /// channel-interleaved order, valid only for the duration of the call.
  virtual EncodeResult encode(const void *data, int numFrames) = 0;

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
    const double sampleRate =
        fileProperties_ ? static_cast<double>(fileProperties_->sampleRate) : 0.0;
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

  std::shared_ptr<AudioFileProperties> fileProperties_;
  StreamFormat inputFormat_;
  EncoderOutputSpec outputSpec_;
  size_t maxBufferSizeInFrames_{0};
  std::string filePath_;
  std::atomic<size_t> framesEncoded_{0};

 private:
  std::atomic<bool> isOpen_{false};
};

} // namespace audioapi
