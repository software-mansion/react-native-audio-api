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
 * MiniAudio-backed incremental decoder (Vorbis/Ogg via libvorbis custom backend).
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
  [[nodiscard]] DecoderResult seekToTime(double seconds) override;

 private:
  void teardownDecoder();

  std::unique_ptr<ma_decoder> decoder_;
  std::vector<uint8_t> memoryCopy_;
};

} // namespace audioapi::decoding::miniaudio
