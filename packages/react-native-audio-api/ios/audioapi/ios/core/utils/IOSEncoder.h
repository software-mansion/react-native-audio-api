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

  EncodeResult encode(const void *data, int numFrames) override;

  CloseEncoderResult close() override;

  [[nodiscard]] size_t getFileSizeBytes() const override;

 private:
  std::unique_ptr<IOSEncoderState> impl_;
};

} // namespace audioapi::ios_encoder
