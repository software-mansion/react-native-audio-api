#pragma once

#include <audioapi/decoding/IncrementalAudioDecoder.h>
#include <audioapi/utils/Macros.h>

#include <cstdint>

namespace audioapi {

/**
 * Shared open-state for OS-native incremental decoders (iOS / Android).
 *
 * Owns the common metadata reset by `close()`; subclasses release platform
 * resources in `releaseImpl()` (e.g. `impl_.reset()`).
 */
class OsDecoderBase : public decoding::IncrementalAudioDecoder {
 public:
  OsDecoderBase() = default;
  ~OsDecoderBase() override = default;
  DELETE_COPY_AND_MOVE(OsDecoderBase);

  void close() final {
    releaseImpl();
    outputChannels_ = 0;
    outputSampleRate_ = 0;
    durationSeconds_ = 0.0;
    framePosition_ = 0;
    open_ = false;
  }

  [[nodiscard]] bool isOpen() const final {
    return open_;
  }

  [[nodiscard]] int outputChannels() const final {
    return outputChannels_;
  }

  [[nodiscard]] int outputSampleRate() const final {
    return outputSampleRate_;
  }

  [[nodiscard]] float getDurationInSeconds() const final {
    return static_cast<float>(durationSeconds_);
  }

  [[nodiscard]] float getCurrentPositionInSeconds() const final {
    if (outputSampleRate_ <= 0) {
      return 0.0f;
    }
    return static_cast<float>(
        static_cast<double>(framePosition_) / static_cast<double>(outputSampleRate_));
  }

 protected:
  /// Drop platform-native decoder resources (ExtAudioFile / MediaCodec, etc.).
  virtual void releaseImpl() = 0;

  int outputChannels_ = 0;
  int outputSampleRate_ = 0;
  double durationSeconds_ = 0.0;
  int64_t framePosition_ = 0;
  bool open_ = false;
};

} // namespace audioapi
