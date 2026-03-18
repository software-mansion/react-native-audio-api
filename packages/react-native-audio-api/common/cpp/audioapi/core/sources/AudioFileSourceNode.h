#pragma once

#include <audioapi/core/sources/AudioScheduledSourceNode.h>
#include <audioapi/libs/ffmpeg/FFmpegDecoding.h>
#include <audioapi/libs/miniaudio/miniaudio.h>

#include <memory>
#include <vector>

namespace audioapi {

struct AudioFileSourceOptions;

struct AudioFileDecoderState {
  ma_decoder decoder;
  std::vector<uint8_t> memoryData;      // keeps memory alive for memory-based decoder
  std::vector<float> interleavedBuffer; // pre-allocated read buffer
  int channels = 0;
  float sampleRate = 0;

  ~AudioFileDecoderState() {
    ma_decoder_uninit(&decoder);
  }
};

class AudioFileSourceNode : public AudioScheduledSourceNode {
 public:
  explicit AudioFileSourceNode(
      const std::shared_ptr<BaseAudioContext> &context,
      const AudioFileSourceOptions &options);
  ~AudioFileSourceNode() override = default;

  /// @note Audio Thread only
  void setDecoderState(const std::shared_ptr<AudioFileDecoderState> &state);

  void disable() override;

 protected:
  std::shared_ptr<DSPAudioBuffer> processNode(
      const std::shared_ptr<DSPAudioBuffer> &processingBuffer,
      int framesToProcess) override;

 private:
  std::shared_ptr<AudioFileDecoderState> decoderState_;
  bool FFmpegNeeded_;
  ffmpegdecoder::FFmpegDecoder decoder;
  ffmpegdecoder::FFmpegDecoderConfig cfg;
};

} // namespace audioapi
