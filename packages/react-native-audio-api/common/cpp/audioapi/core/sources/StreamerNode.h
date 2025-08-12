#pragma once

#include <audioapi/core/sources/AudioScheduledSourceNode.h>

extern "C" {
  #include <libavformat/avformat.h>
  #include <libavcodec/avcodec.h>
  #include <libavutil/samplefmt.h>
  #include <libavutil/channel_layout.h>
  #include <libavutil/opt.h>
  #include <libswresample/swresample.h>
}

#include <cmath>
#include <memory>
#include <string>
#include <atomic>

namespace audioapi {

class AudioBus;

class StreamerNode : public AudioScheduledSourceNode {
 public:
  explicit StreamerNode(BaseAudioContext *context);
  ~StreamerNode() override;
  bool initialize(const std::string& inputUrl);
  void startStreaming();
  void stopStreaming();

 protected:
  void processNode(const std::shared_ptr<AudioBus>& processingBus, int framesToProcess) override;

 private:
  std::string streamPath_;
  AVFormatContext* fmtCtx_;
  AVCodecContext* codecCtx_;
  const AVCodec* decoder_;
  AVCodecParameters* codecpar_;
  AVPacket* pkt_;
  AVFrame* frame_;
  AVFrame* pendingFrame_;
  bool hasPendingFrame_;
  std::shared_ptr<AudioBus> bufferedBus_;
  size_t bufferedBusIndex_;
  size_t maxBufferSize_;
  int audio_stream_index_;

  SwrContext* swrCtx_;
  uint8_t** resampledData_;
  int maxResampledSamples_;
  std::mutex mutex_;
  std::thread thread_;
  std::atomic<bool> streamFlag;

  // Add these private methods:
  bool setupResampler();
  bool processFrameWithResampler(AVFrame* frame);
  void streamAudio();

  // Private helper methods
  void cleanup();
  bool openInput(const std::string& inputUrl);
  bool findAudioStream();
  bool setupDecoder();
};
} // namespace audioapi
