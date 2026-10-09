#pragma once

#include <audioapi/encoding/AudioEncoder.h>
#include <audioapi/utils/Result.hpp>

#include <memory>
#include <string>
#include "audioapi/utils/Macros.h"

namespace audioapi::android::encoder {

class IEncoderBackend;

/// Android system-API encoder (RIFF / MediaCodec + MediaMuxer).
class AndroidEncoder : public AudioEncoder {
 public:
  explicit AndroidEncoder(const EncoderSettings &settings);
  ~AndroidEncoder() override;
  DELETE_COPY_AND_MOVE(AndroidEncoder);

  OpenEncoderResult open(
      const StreamFormat &inputFormat,
      const EncoderOutputSpec &outputSpec,
      const std::string &filePath) override;

  EncodeResult encode(const float *const *channels, int numFrames) override;

  CloseEncoderResult close() override;

  [[nodiscard]] size_t getFileSizeBytes() const override;

 protected:
  /// Only the conversion stage is rebuilt; the backend keeps writing at outputLayout_.
  OpenEncoderResult reprepareInput(const StreamFormat &inputFormat) override;

 private:
  static constexpr int RESAMPLE_MAX_IN_FRAMES = 4096;

  struct ConversionState;

  /// Sets conversion_ to what converts frames from @p inputFormat to the backend's effective
  /// format (outputLayout_), or clears it when the two already match.
  [[nodiscard]] Result<NoneType, std::string> prepareConversion(const StreamFormat &inputFormat);

  /// Input that differs from the backend's effective format: channel mapping and resampling
  /// stay planar, and the backend interleaves while it quantizes.
  std::string encodeConverted(const float *const *channels, int numFrames);

  std::unique_ptr<IEncoderBackend> backend_;

  /// What the backend encodes at; the input differs from it only when conversion_ is set.
  AudioLayout outputLayout_;

  std::unique_ptr<ConversionState> conversion_; // null when no conversion is needed
};

} // namespace audioapi::android::encoder
