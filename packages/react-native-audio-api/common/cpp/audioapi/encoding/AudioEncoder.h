#pragma once

#include <audioapi/encoding/EncoderOutputSpec.h>
#include <audioapi/encoding/StreamFormat.h>
#include <audioapi/utils/AudioFileProperties.h>
#include <audioapi/utils/AudioLayout.h>
#include <audioapi/utils/Macros.h>
#include <audioapi/utils/Result.hpp>

#include <atomic>
#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <tuple>

namespace audioapi {

using OpenEncoderResult = Result<std::string, std::string>;
using EncodeResult = Result<size_t, std::string>;
using CloseEncoderResult = Result<std::tuple<double, double>, std::string>;

class AudioEncoder;
using CreateEncoderResult = Result<std::unique_ptr<AudioEncoder>, std::string>;

/// What an encoder writes. The rest of AudioFileProperties (where the file goes, how a
/// recording rotates) is the writer's business, so encoders never see it.
struct EncoderSettings {
  /// Sample rate and channel count of the file; the encoder converts its input to them.
  AudioLayout fileLayout;
  AudioFileProperties::EncodingConfig encoding;
};

/// Builds an encoder that is not open yet; the caller opens it on the output path.
using EncoderFactory = std::function<CreateEncoderResult(const EncoderSettings &)>;

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

  /// Keeps writing the same file from input in @p inputFormat.
  OpenEncoderResult changeInputFormat(const StreamFormat &inputFormat) {
    // Measured before the hook runs, since implementations overwrite inputFormat_ in it.
    const double durationSoFar = getEncodedDurationSeconds();
    auto result = reprepareInput(inputFormat);
    if (result.is_ok()) {
      earlierFormatsDurationSec_ = durationSoFar;
      framesEncoded_.store(0, std::memory_order_release);
      inputFormat_ = inputFormat;
    }
    return result;
  }

  [[nodiscard]] bool isOpen() const {
    return isOpen_.load(std::memory_order_acquire);
  }
  [[nodiscard]] const std::string &getFilePath() const {
    return filePath_;
  }
  [[nodiscard]] virtual size_t getFileSizeBytes() const = 0;

  /// Of the input encoded so far, each frame measured at the sample rate it arrived in.
  [[nodiscard]] double getEncodedDurationSeconds() const {
    const auto inputSampleRate = static_cast<double>(inputFormat_.layout.sampleRate);
    if (inputSampleRate <= 0.0) {
      return earlierFormatsDurationSec_;
    }
    return earlierFormatsDurationSec_ +
        static_cast<double>(framesEncoded_.load(std::memory_order_acquire)) / inputSampleRate;
  }

 protected:
  /// Rebuilds whatever the implementation derives from the input format while the output file
  /// stays open. Fails, leaving the file as it was, when that is not possible.
  virtual OpenEncoderResult reprepareInput(const StreamFormat &inputFormat) = 0;

  void markOpen() {
    isOpen_.store(true, std::memory_order_release);
  }
  void markClosed() {
    isOpen_.store(false, std::memory_order_release);
  }

  /// For open(), so a reused encoder starts the next file from zero.
  void resetEncodedDuration() {
    framesEncoded_.store(0, std::memory_order_release);
    earlierFormatsDurationSec_ = 0.0;
  }

  /// @p inputFrames as handed to encode(), before any resampling.
  void addEncodedFrames(size_t inputFrames) {
    framesEncoded_.fetch_add(inputFrames, std::memory_order_acq_rel);
  }

  EncoderSettings settings_;
  StreamFormat inputFormat_;
  EncoderOutputSpec outputSpec_;
  std::string filePath_;

 private:
  std::atomic<bool> isOpen_{false};
  /// Written on the JS thread, read from the file-writer worker under the writer's mutex.
  std::atomic<size_t> framesEncoded_{0};
  double earlierFormatsDurationSec_{0.0};
};

} // namespace audioapi
