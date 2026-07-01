#pragma once

#include <audioapi/libs/decoding/IncrementalAudioDecoder.h>
#include <audioapi/libs/miniaudio/miniaudio.h>
#include <audioapi/utils/AudioBuffer.hpp>
#include <audioapi/utils/Macros.h>

#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace audioapi::miniaudio_decoder {

/**
 * MiniAudio-backed incremental decoder (Vorbis/Opus/WAV, etc. via ma_decoder + custom backends).
 * Same usage contract as ffmpeg_decoder::FFmpegDecoder.
 */
class MiniAudioDecoder : public decoding::IncrementalAudioDecoder {
 public:
  MiniAudioDecoder() = default;
  ~MiniAudioDecoder() override;
  DELETE_COPY_AND_MOVE(MiniAudioDecoder);

  [[nodiscard]] decoding::DecoderResult openFile(
      int outputSampleRate,
      const std::string &path) override;
  [[nodiscard]] decoding::DecoderResult openMemory(
      int outputSampleRate,
      const void *data,
      size_t size) override;
  [[nodiscard]] decoding::DecoderResult openUrl(
      int outputSampleRate,
      const std::string &url,
      const std::map<std::string, std::string> &headers = {}) override;
  [[nodiscard]] size_t readPcmFrames(float *outInterleaved, size_t frameCount) override;
  void close() override;
  [[nodiscard]] bool isOpen() const override;
  [[nodiscard]] int outputChannels() const override;
  [[nodiscard]] int outputSampleRate() const override;
  [[nodiscard]] float getDurationInSeconds() const override;
  [[nodiscard]] float getCurrentPositionInSeconds() const override;
  [[nodiscard]] decoding::DecoderResult seekToTime(double seconds) override;

 private:
  void teardownDecoder();

  std::unique_ptr<ma_decoder> decoder_;
  std::vector<uint8_t> memoryCopy_;
  int outputChannels_ = 0;
  int outputSampleRate_ = 0;
  size_t totalOutputFrames_ = 0;
  std::uint64_t totalLengthFrames_ = 0;
};

std::shared_ptr<AudioBuffer> decodeWithMemoryBlock(const void *data, size_t size, int sample_rate);
std::shared_ptr<AudioBuffer> decodeWithFilePath(const std::string &path, int sample_rate);

} // namespace audioapi::miniaudio_decoder
