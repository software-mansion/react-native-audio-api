#pragma once

#include <audioapi/android/core/utils/AndroidFileWriterBackend.h>
#include <audioapi/android/core/utils/ffmpegBackend/utils.h>
#include <string>
#include <memory>
#include <tuple>
#include <chrono>
#include <audioapi/utils/Result.hpp>

struct AVCodecContext;
struct AVFormatContext;
struct AVFrame;
struct AVPacket;
struct AVAudioFifo;
struct SwrContext;
struct AVStream;

namespace audioapi {

class AudioFileProperties;

namespace android::ffmpeg {

class FFmpegAudioFileWriter : public AndroidFileWriterBackend {
 public:
  explicit FFmpegAudioFileWriter(
      const std::shared_ptr<AudioEventHandlerRegistry> &audioEventHandlerRegistry,
      const std::shared_ptr<AudioFileProperties> &fileProperties);
  ~FFmpegAudioFileWriter();

  OpenFileResult openFile(float streamSampleRate, int32_t streamChannelCount, int32_t streamMaxBufferSize, const std::string &fileNameOverride) override;
  CloseFileResult closeFile() override;

  bool writeAudioData(void *data, int numFrames) override;

 private:
  av_unique_ptr<AVCodecContext> encoderCtx_{nullptr};
  av_unique_ptr<AVFormatContext> formatCtx_{nullptr};
  av_unique_ptr<SwrContext> resampleCtx_{nullptr};
  av_unique_ptr<AVAudioFifo> audioFifo_{nullptr};
  av_unique_ptr<AVPacket> packet_{nullptr};
  av_unique_ptr<AVFrame> resamplerFrame_{nullptr};
  av_unique_ptr<AVFrame> writingFrame_{nullptr};
  AVStream* stream_{nullptr};

  unsigned int nextPts_{0};

  std::chrono::steady_clock::time_point lastFlushTime_ = std::chrono::steady_clock::now();
  int flushIntervalMs_;

  // Initialization helper methods
  Result<NoneType, std::string> initializeFormatContext(const AVCodec* codec);
  Result<NoneType, std::string> configureAndOpenCodec(const AVCodec* codec);
  Result<NoneType, std::string> initializeStream();
  Result<NoneType, std::string> openIOAndWriteHeader();
  Result<NoneType, std::string> initializeResampler(float inputRate, int inputChannels);
  void initializeBuffers(int32_t maxBufferSize);

  // Processing helper methods
  bool resampleAndPushToFifo(void *data, int numFrames);
  int processFifo(bool flush);
  int writeEncodedPackets();

  // Finalization helper methods
  CloseFileResult finalizeOutput();
};

} // namespace android::ffmpeg

} // namespace audioapi
