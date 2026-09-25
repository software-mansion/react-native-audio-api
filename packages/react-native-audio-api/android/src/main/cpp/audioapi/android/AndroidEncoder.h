#pragma once

#include <audioapi/encoding/AudioEncoder.h>
#include <audioapi/utils/Result.hpp>

#include <memory>
#include <string>

namespace audioapi::android_encoder {

class IEncoderBackend;

/// Android system-API encoder (RIFF / MediaCodec + MediaMuxer).
class AndroidEncoder : public AudioEncoder {
 public:
  explicit AndroidEncoder(const std::shared_ptr<AudioFileProperties> &fileProperties);
  ~AndroidEncoder() override;

  OpenEncoderResult open(
      const StreamFormat &inputFormat,
      const EncoderOutputSpec &outputSpec,
      size_t maxBufferSizeInFrames,
      const std::string &filePath) override;

  EncodeResult encode(const float *const *channels, int numFrames) override;

  CloseEncoderResult close() override;

  [[nodiscard]] size_t getFileSizeBytes() const override;

 private:
  /// Input that differs from the backend's effective format: channel mapping and resampling
  /// stay planar, and the backend interleaves while it quantizes.
  std::string encodeConverted(const float *const *channels, int numFrames);

  std::unique_ptr<IEncoderBackend> backend_;

  double inputSampleRate_{0.0};
  double outputSampleRate_{0.0};
  int inputChannelCount_{0};
  int outputChannelCount_{0};

  struct ConversionState;
  std::unique_ptr<ConversionState> conversion_; // null when no conversion is needed
};

} // namespace audioapi::android_encoder
