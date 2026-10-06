#pragma once

#include <audioapi/encoding/AudioEncoder.h>
#include <audioapi/utils/Result.hpp>

#include <memory>
#include <string>

namespace audioapi::ios::encoder {

struct IOSEncoderState;

/// iOS system-API encoder backed by AVAudioFile + AVAudioConverter.
class IOSEncoder : public AudioEncoder {
 public:
  explicit IOSEncoder(const EncoderSettings &settings);
  ~IOSEncoder() override;

  OpenEncoderResult open(
      const StreamFormat &inputFormat,
      const EncoderOutputSpec &outputSpec,
      const std::string &filePath) override;

  EncodeResult encode(const float *const *channels, int numFrames) override;

  CloseEncoderResult close() override;

  [[nodiscard]] size_t getFileSizeBytes() const override;

 protected:
  /// Only the converter, which is built for the input, is rebuilt; the output file stays open.
  OpenEncoderResult reprepareInput(const StreamFormat &inputFormat) override;

 private:
  /// Builds the input format, converter and conversion buffers for an already open file.
  Result<NoneType, std::string> prepareConversionPipeline(const StreamFormat &inputFormat);
  void releaseConversionPipeline();

  std::unique_ptr<IOSEncoderState> state_;
};

} // namespace audioapi::ios::encoder
