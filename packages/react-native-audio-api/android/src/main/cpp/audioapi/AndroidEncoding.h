#pragma once

#include <audioapi/encoding/AudioEncoder.h>
#include <audioapi/utils/Result.hpp>

#include <memory>
#include <string>
#include <vector>

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

  EncodeResult encode(const void *data, int numFrames) override;

  CloseEncoderResult close() override;

  [[nodiscard]] size_t getFileSizeBytes() const override;

 private:
  int convertToOutput(const float *input, int numFrames);

  std::unique_ptr<IEncoderBackend> backend_;
  std::vector<float> channelBuffer_;
  std::vector<float> convertedBuffer_;

  double inputSampleRate_{0.0};
  double outputSampleRate_{0.0};
  int inputChannelCount_{0};
  int outputChannelCount_{0};

  struct ResamplerState;
  std::unique_ptr<ResamplerState> resampler_;
};

} // namespace audioapi::android_encoder
