#pragma once

#include <audioapi/encoding/AudioEncoder.h>
#include <audioapi/utils/Result.hpp>

#include <memory>
#include <string>

namespace audioapi::ios_encoder {

struct IOSEncoderState;

/// iOS system-API encoder backed by AVAudioFile + AVAudioConverter.
class IOSEncoder : public AudioEncoder {
 public:
  explicit IOSEncoder(const std::shared_ptr<AudioFileProperties> &fileProperties);
  ~IOSEncoder() override;

  OpenEncoderResult open(
      const StreamFormat &inputFormat,
      const EncoderOutputSpec &outputSpec,
      size_t maxBufferSizeInFrames,
      const std::string &filePath) override;

  /// Switches an open encoder to a new input format without touching the output file: only
  /// the converter, which is built for the input, is rebuilt.
  OpenEncoderResult reprepareInput(const StreamFormat &inputFormat, size_t maxBufferSizeInFrames);

  EncodeResult encode(const float *const *channels, int numFrames) override;

  CloseEncoderResult close() override;

  [[nodiscard]] size_t getFileSizeBytes() const override;

 private:
  /// Builds the input format, converter and conversion buffers for an already open file.
  Result<NoneType, std::string> prepareConversionPipeline(
      const StreamFormat &inputFormat,
      size_t maxBufferSizeInFrames);
  void releaseConversionPipeline();

  std::unique_ptr<IOSEncoderState> impl_;
};

} // namespace audioapi::ios_encoder
