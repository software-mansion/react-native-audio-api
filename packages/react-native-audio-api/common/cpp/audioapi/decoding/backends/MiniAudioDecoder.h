#pragma once

#include <audioapi/decoding/DecoderSource.h>
#include <audioapi/decoding/backends/AudioDecoderBackend.h>
#include <audioapi/libs/miniaudio/miniaudio.h>
#include <audioapi/utils/Macros.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

namespace audioapi::decoding::miniaudio {

/**
 * MiniAudio-backed incremental decoder (Vorbis/Opus/WAV, etc. via ma_decoder + custom backends).
 * Local file / memory only — remote URLs use FfmpegDecoder.
 */
class MiniAudioDecoder : public AudioDecoderBackend {
 public:
  MiniAudioDecoder() = default;
  ~MiniAudioDecoder() override;
  DELETE_COPY_AND_MOVE(MiniAudioDecoder);

  [[nodiscard]] DecoderResult open(const LocalFileSource &source);
  [[nodiscard]] DecoderResult open(const EncodedMemorySource &source);

  [[nodiscard]] size_t readPcmFrames(float *outInterleaved, size_t frameCount) override;
  void close() override;
  [[nodiscard]] bool isOpen() const override;
  [[nodiscard]] int outputChannels() const override;
  [[nodiscard]] int outputSampleRate() const override;
  [[nodiscard]] float getDurationInSeconds() const override;
  [[nodiscard]] float getCurrentPositionInSeconds() const override;
  [[nodiscard]] DecoderResult seekToTime(double seconds) override;
  [[nodiscard]] size_t getTotalPcmFrameCount() const override;

 private:
  void teardownDecoder();

  std::unique_ptr<ma_decoder> decoder_;
  std::vector<uint8_t> memoryCopy_;
  int outputChannels_ = 0;
  int outputSampleRate_ = 0;
  size_t totalOutputFrames_ = 0;
  std::uint64_t totalLengthFrames_ = 0;
};

} // namespace audioapi::decoding::miniaudio
