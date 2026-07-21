#pragma once

#include <audioapi/decoding/DecoderSource.h>
#include <audioapi/decoding/backends/AudioDecoderBackend.h>
#include <audioapi/utils/Macros.h>

#include <cstddef>
#include <vector>

namespace audioapi::decoding::raw_pcm {

class RawPcmDecoder : public AudioDecoderBackend {
 public:
  RawPcmDecoder() = default;
  ~RawPcmDecoder() override = default;
  DELETE_COPY_AND_MOVE(RawPcmDecoder);

  [[nodiscard]] DecoderResult open(const RawPcmSource &source);
  [[nodiscard]] DecoderResult open(const RawPcmBase64Source &source);

  [[nodiscard]] size_t readPcmFrames(float *outInterleaved, size_t frameCount) override;
  [[nodiscard]] DecoderResult seekToTime(double seconds) override;
  void close() override;
  [[nodiscard]] bool isOpen() const override;
  [[nodiscard]] int outputChannels() const override;
  [[nodiscard]] int outputSampleRate() const override;
  [[nodiscard]] float getDurationInSeconds() const override;
  [[nodiscard]] float getCurrentPositionInSeconds() const override;
  [[nodiscard]] size_t getTotalPcmFrameCount() const override;

 private:
  [[nodiscard]] DecoderResult openFromBytes(
      const uint8_t *data,
      size_t size,
      int sampleRate,
      int channelCount,
      bool interleaved);

  std::vector<float> interleavedPcm_;
  size_t framePosition_ = 0;
  int outputChannels_ = 0;
  int outputSampleRate_ = 0;
};

} // namespace audioapi::decoding::raw_pcm
