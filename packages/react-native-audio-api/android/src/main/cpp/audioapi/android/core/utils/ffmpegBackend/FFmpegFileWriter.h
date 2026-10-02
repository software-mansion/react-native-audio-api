#pragma once

#include <audioapi/android/core/utils/AndroidFileWriterBackend.h>
#include <audioapi/android/core/utils/ffmpegBackend/utils.h>
#include <audioapi/utils/Result.hpp>
#include <chrono>
#include <memory>
#include <string>

struct AVCodecContext;
struct AVFormatContext;
struct AVFrame;
struct AVPacket;
struct AVAudioFifo;
struct SwrContext;
struct AVStream;
struct AVIOContext;

namespace audioapi {

class AudioFileProperties;

namespace android::ffmpeg {

class FFmpegAudioFileWriter : public AndroidFileWriterBackend {
 public:
  explicit FFmpegAudioFileWriter(
      const std::shared_ptr<IAudioEventHandlerRegistry> &audioEventHandlerRegistry,
      const std::shared_ptr<AudioFileProperties> &fileProperties,
      float streamSampleRate,
      int32_t streamChannelCount,
      int32_t streamMaxBufferSize);
  ~FFmpegAudioFileWriter();

  size_t getFileSizeBytes() const override;

  OpenFileResult openFile(
      float streamSampleRate,
      int32_t streamChannelCount,
      int32_t streamMaxBufferSize,
      const std::string &fileNameOverride) override;
  CloseFileResult closeFile() override;

 private:
  av_unique_ptr<AVCodecContext> encoderCtx_{nullptr};
  av_unique_ptr<AVFormatContext> formatCtx_{nullptr};
  av_unique_ptr<SwrContext> resampleCtx_{nullptr};
  av_unique_ptr<AVAudioFifo> audioFifo_{nullptr};
  av_unique_ptr<AVPacket> packet_{nullptr};
  av_unique_ptr<AVFrame> resamplerFrame_{nullptr};
  av_unique_ptr<AVFrame> writingFrame_{nullptr};
  AVStream *stream_{nullptr};

  // ADTS output bypasses the muxer layer entirely: the prebuilt FFmpeg is
  // built without the adts muxer (rn-audio-libs: --enable-muxer=wav,mp4,flac,caf),
  // and ADTS needs none — each frame is a 7-byte header + raw AAC packet,
  // written straight to this AVIOContext.
  bool isAdts_{false};
  AVIOContext *adtsIo_{nullptr};

  unsigned int nextPts_{0};

  std::chrono::steady_clock::time_point lastFlushTime_ = std::chrono::steady_clock::now();
  int flushIntervalMs_;

  // Initialization helper methods
  Result<NoneType, std::string> initializeFormatContext(const AVCodec *codec);
  Result<NoneType, std::string> configureAndOpenCodec(const AVCodec *codec);
  Result<NoneType, std::string> initializeStream();
  Result<NoneType, std::string> openIOAndWriteHeader();
  Result<NoneType, std::string> openAdtsIO();
  // TODO: rewrite to use r8brain resampler
  Result<NoneType, std::string> initializeResampler(float inputRate, int inputChannels);
  void initializeBuffers(int32_t maxBufferSize);

  // Processing helper methods
  bool resampleAndPushToFifo(void *data, int numFrames);
  int processFifo(bool flush);
  int writeEncodedPackets();
  int writeAdtsPacket();

  // Finalization helper methods
  CloseFileResult finalizeOutput();
  void rollbackFailedOpen();
  void processWriterData(void *data, int numFrames) override;
};

} // namespace android::ffmpeg

} // namespace audioapi
