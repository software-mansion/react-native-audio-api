#if !RN_AUDIO_API_FFMPEG_DISABLED

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/audio_fifo.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>
}

#include <audioapi/android/core/utils/AndroidFileWriterBackend.h>
#include <audioapi/android/core/utils/FileOptions.h>
#include <audioapi/android/core/utils/ffmpegBackend/FFmpegFileWriter.h>
#include <audioapi/android/core/utils/ffmpegBackend/ptrs.hpp>
#include <audioapi/android/core/utils/ffmpegBackend/utils.h>
#include <audioapi/utils/AudioFileProperties.h>
#include <audioapi/utils/UnitConversion.h>

#include <algorithm>
#include <cassert>
#include <memory>
#include <string>
#include <utility>

constexpr int fallbackFIFOSize = 8192;
constexpr int fallbackFrameSize = 512;
constexpr int defaultFlushInterval = 100;

namespace audioapi::android::ffmpeg {

FFmpegAudioFileWriter::FFmpegAudioFileWriter(
    const std::shared_ptr<AudioEventHandlerRegistry> &audioEventHandlerRegistry,
    const std::shared_ptr<AudioFileProperties> &fileProperties)
    : AndroidFileWriterBackend(audioEventHandlerRegistry, fileProperties) {
  // Set flush interval from properties, limit minimum to 100ms
  // to avoid people hurting themselves too much
  flushIntervalMs_ = std::max(fileProperties_->androidFlushIntervalMs, defaultFlushInterval);
}

FFmpegAudioFileWriter::~FFmpegAudioFileWriter() {
  if (isFileOpen()) {
    closeFile();
  }
}

/// @brief Opens a specified audio file for writing and prepares any necessary resources.
/// such as codecs, conversion buffers or circular AVIO FIFO.
/// This method should be called from the JS thread only.
/// @param streamSampleRate The sample rate of the incoming audio stream (aka microphone).
/// @param streamChannelCount The number of channels in the incoming audio stream.
/// @param streamMaxBufferSize The estimated maximum buffer size for the incoming audio stream.
/// @returns Success status with file path or Error status with message.
OpenFileResult FFmpegAudioFileWriter::openFile(
    float streamSampleRate,
    int32_t streamChannelCount,
    int32_t streamMaxBufferSize,
    const std::string &fileNameOverride) {
  streamSampleRate_ = streamSampleRate;
  streamChannelCount_ = streamChannelCount;
  streamMaxBufferSize_ = streamMaxBufferSize;
  framesWritten_.store(0, std::memory_order_release);
  nextPts_ = 0;
  Result<NoneType, std::string> result = Result<NoneType, std::string>::Ok(None);
  Result<std::string, std::string> filePathResult =
      fileoptions::getFilePath(fileProperties_, fileNameOverride);

  if (!filePathResult.is_ok()) {
    return OpenFileResult::Err(filePathResult.unwrap_err());
  }

  filePath_ = filePathResult.unwrap();

  const AVCodec *codec = getCodec(fileProperties_);

  if (!codec) {
    return OpenFileResult::Err("Unsupported codec for the given file format");
  }

  return initializeFormatContext(codec)
      .and_then([this, codec](auto) { return configureAndOpenCodec(codec); })
      .and_then([this](auto) { return initializeStream(); })
      .and_then([this](auto) { return openIOAndWriteHeader(); })
      .and_then([this, streamSampleRate, streamChannelCount](auto) {
        return initializeResampler(streamSampleRate, streamChannelCount);
      })
      .and_then([this, streamMaxBufferSize, filePath = std::move(filePath_)](auto) {
        initializeBuffers(streamMaxBufferSize);
        isFileOpen_.store(true, std::memory_order_release);
        return OpenFileResult::Ok(filePath);
      });
}

/// @brief Closes the currently opened audio file, flushing any remaining data and finalizing the file.
/// This method should called from the JS thread only.
/// @returns CloseFileStatus indicating success with file path, size and duration, or error with message.
CloseFileResult FFmpegAudioFileWriter::closeFile() {
  int result = 0;

  if (!isFileOpen()) {
    return CloseFileResult::Err("File is not open");
  }

  result = processFifo(true);

  if (result < 0) {
    auto finalStatus = finalizeOutput();

    return CloseFileResult::Err(
        "Failed to flush FIFO to encoder. error code: " + parseErrorCode(result) +
        ", finalization status: " + (finalStatus.is_ok() ? "success" : finalStatus.unwrap_err()));
  }

  result = avcodec_send_frame(encoderCtx_.get(), nullptr);

  if (result < 0) {
    return CloseFileResult::Err("Failed to send EOF to encoder");
  }

  if (writeEncodedPackets() < 0) {
    return CloseFileResult::Err("Failed to drain encoder packets");
  }

  return finalizeOutput();
}

/// @brief Writes audio data to the currently opened file.
/// This method should be called only from the audio thread (or audio side-effect thread in the future).
/// @param data Pointer to the audio data buffer (interleaved float samples) as returned by Oboe stream.
/// @param numFrames Number of audio frames in the data buffer.
/// @returns True if the data was written successfully, false otherwise.
bool FFmpegAudioFileWriter::writeAudioData(void *data, int numFrames) {
  if (!isFileOpen()) {
    return false;
  }

  if (!resampleAndPushToFifo(data, numFrames)) {
    return false;
  }

  framesWritten_.fetch_add(numFrames, std::memory_order_acq_rel);

  if (processFifo(false) < 0) {
    return false;
  }

  return true;
}

/// @brief Initializes the FFmpeg format context for the output file.
/// @param codec The codec to be used for encoding.
/// @returns Success status or Error status with message.
Result<NoneType, std::string> FFmpegAudioFileWriter::initializeFormatContext(const AVCodec *codec) {
  AVFormatContext *rawFormatCtx = nullptr;

  int result = avformat_alloc_output_context2(
      &rawFormatCtx, nullptr, getMuxerName(fileProperties_).c_str(), filePath_.c_str());

  if (result < 0 || !rawFormatCtx) {
    return Result<NoneType, std::string>::Err(
        "Failed to allocate FFmpeg format context with error: " + parseErrorCode(result));
  }

  formatCtx_ = av_unique_ptr<AVFormatContext>(rawFormatCtx);
  return Result<NoneType, std::string>::Ok(None);
}

/// @brief Configures and opens the codec context for encoding.
/// @param codec The codec to be used for encoding.
/// @returns Success status or Error status with message.
Result<NoneType, std::string> FFmpegAudioFileWriter::configureAndOpenCodec(const AVCodec *codec) {
  encoderCtx_ = av_unique_ptr<AVCodecContext>(avcodec_alloc_context3(codec));

  if (!encoderCtx_) {
    return Result<NoneType, std::string>::Err("Failed to allocate FFmpeg codec context");
  }

  av_channel_layout_default(&encoderCtx_->ch_layout, fileProperties_->channelCount);
  encoderCtx_->sample_rate = static_cast<int>(fileProperties_->sampleRate);
  encoderCtx_->sample_fmt = getSampleFormat(fileProperties_);

  if (fileProperties_->bitRate > 0) {
    encoderCtx_->bit_rate = fileProperties_->bitRate;
  }

  AVDictionary *codecOptions = nullptr;

  if (fileProperties_->flacCompressionLevel >= 0) {
    av_dict_set_int(&codecOptions, "compression_level", fileProperties_->flacCompressionLevel, 0);
  }

  int result = avcodec_open2(encoderCtx_.get(), codec, &codecOptions);
  av_dict_free(&codecOptions);

  if (result < 0) {
    return Result<NoneType, std::string>::Err(
        "Failed to open FFmpeg codec with error: " + parseErrorCode(result));
  }

  return Result<NoneType, std::string>::Ok(None);
}

/// @brief Initializes a new stream in the format context.
/// @returns Success status or Error status with message.
Result<NoneType, std::string> FFmpegAudioFileWriter::initializeStream() {
  stream_ = avformat_new_stream(formatCtx_.get(), nullptr);

  if (!stream_) {
    return Result<NoneType, std::string>::Err("Failed to create new stream in format context");
  }

  int result = avcodec_parameters_from_context(stream_->codecpar, encoderCtx_.get());

  if (result < 0) {
    return Result<NoneType, std::string>::Err(
        "Failed to copy codec parameters to stream with error: " + parseErrorCode(result));
  }

  stream_->time_base = AVRational{1, static_cast<int>(encoderCtx_->sample_rate)};
  return Result<NoneType, std::string>::Ok(None);
}

/// @brief Opens the file and writes the basic header (depends on the codec/format used).
/// @returns Success status or Error status with message.
Result<NoneType, std::string> FFmpegAudioFileWriter::openIOAndWriteHeader() {
  int result = 0;

  if (!(formatCtx_->oformat->flags & AVFMT_NOFILE)) {
    result = avio_open(&formatCtx_->pb, filePath_.c_str(), AVIO_FLAG_WRITE);

    if (result < 0) {
      return Result<NoneType, std::string>::Err(
          "Failed to open output file with error: " + parseErrorCode(result));
    }
  }

  result = avformat_write_header(formatCtx_.get(), nullptr);

  if (result < 0) {
    return Result<NoneType, std::string>::Err("Failed to write header to file: " + filePath_);
  }

  return Result<NoneType, std::string>::Ok(None);
}

/// @brief Initializes the resampler context for audio conversion.
/// @param inputRate The sample rate of the input audio.
/// @param inputChannels The number of channels in the input audio.
/// @returns Success status or Error status with message.
Result<NoneType, std::string> FFmpegAudioFileWriter::initializeResampler(
    float inputRate,
    int inputChannels) {
  resampleCtx_ = av_unique_ptr<SwrContext>(swr_alloc());

  if (!resampleCtx_) {
    return Result<NoneType, std::string>::Err("Failed to allocate resampler context");
  }

  AVChannelLayout inChannelLayout;
  av_channel_layout_default(&inChannelLayout, inputChannels);

  av_opt_set_chlayout(resampleCtx_.get(), "in_chlayout", &inChannelLayout, 0);
  av_opt_set_chlayout(resampleCtx_.get(), "out_chlayout", &encoderCtx_->ch_layout, 0);

  av_opt_set_int(resampleCtx_.get(), "in_sample_rate", static_cast<int64_t>(inputRate), 0);
  av_opt_set_int(resampleCtx_.get(), "out_sample_rate", encoderCtx_->sample_rate, 0);

  av_opt_set_sample_fmt(resampleCtx_.get(), "in_sample_fmt", AV_SAMPLE_FMT_FLT, 0);
  av_opt_set_sample_fmt(resampleCtx_.get(), "out_sample_fmt", encoderCtx_->sample_fmt, 0);

  int result = swr_init(resampleCtx_.get());

  if (result < 0) {
    return Result<NoneType, std::string>::Err(
        "Failed to initialize resampler for file: " + parseErrorCode(result));
  }

  return Result<NoneType, std::string>::Ok(None);
}

/// @brief Initializes frame and packet buffers as well as the audio FIFO,
/// that might be needed for storing intermediate audio data or buffering before encoding.
/// @param maxBufferSize The maximum buffer size to allocate.
void FFmpegAudioFileWriter::initializeBuffers(int32_t maxBufferSize) {
  resamplerFrame_ = av_unique_ptr<AVFrame>(av_frame_alloc());
  writingFrame_ = av_unique_ptr<AVFrame>(av_frame_alloc());
  packet_ = av_unique_ptr<AVPacket>(av_packet_alloc());

  // Calculate resampler size of output buffer from the resampler
  int resamplerFrameSize = av_rescale_rnd(
      maxBufferSize,
      static_cast<int>(encoderCtx_->sample_rate),
      static_cast<int>(streamSampleRate_),
      AV_ROUND_UP);

  // Configure frame parameters for desired file output
  resamplerFrame_->nb_samples = resamplerFrameSize;
  resamplerFrame_->format = encoderCtx_->sample_fmt;
  av_channel_layout_copy(&resamplerFrame_->ch_layout, &encoderCtx_->ch_layout);
  // Allocate buffer for the resampler frame
  av_frame_get_buffer(resamplerFrame_.get(), 0);

  // calculate FIFO size based on max buffer size and encoder frame size
  // max(2 * resamplerFrameSize, 2 * encoderCtx_->frame_size, fallbackFIFOSize)
  int writingFrameSize = 2 * std::max(encoderCtx_->frame_size, fallbackFrameSize);
  int fifoSize = std::max(std::max(2 * resamplerFrameSize, writingFrameSize), fallbackFIFOSize);

  audioFifo_ = av_unique_ptr<AVAudioFifo>(
      av_audio_fifo_alloc(encoderCtx_->sample_fmt, encoderCtx_->ch_layout.nb_channels, fifoSize));

  // Configure writing frame parameters
  // size 2 x encoder frame size + same format as encoder
  writingFrame_->nb_samples = writingFrameSize;
  av_channel_layout_copy(&writingFrame_->ch_layout, &encoderCtx_->ch_layout);
  writingFrame_->format = encoderCtx_->sample_fmt;
  writingFrame_->sample_rate = encoderCtx_->sample_rate;
  // Allocate buffer for the writing frame
  av_frame_get_buffer(writingFrame_.get(), 0);
}

/// @brief Resamples input audio data and pushes it to the audio FIFO.
/// @param inputData Pointer to the input audio data.
/// @param inputFrameCount Number of input frames.
/// @returns True if successful, false otherwise.
bool FFmpegAudioFileWriter::resampleAndPushToFifo(void *inputData, int inputFrameCount) {
  int64_t outputLength = av_rescale_rnd(
      inputFrameCount, encoderCtx_->sample_rate, static_cast<int>(streamSampleRate_), AV_ROUND_UP);

  const uint8_t *inputs[1] = {reinterpret_cast<const uint8_t *>(inputData)};

  assert(outputLength <= resamplerFrame_->nb_samples);

  int convertedSamples = swr_convert(
      resampleCtx_.get(),
      resamplerFrame_->data,
      static_cast<int>(outputLength),
      inputs,
      inputFrameCount);

  if (convertedSamples < 0) {
    invokeOnErrorCallback("Failed to convert audio samples: " + parseErrorCode(convertedSamples));
    return false;
  }

  int written = av_audio_fifo_write(
      audioFifo_.get(), reinterpret_cast<void **>(resamplerFrame_->data), convertedSamples);

  if (written < convertedSamples) {
    invokeOnErrorCallback("Failed to write all samples to FIFO");
    return false;
  }

  return true;
}

/// @brief pushes the audio data from FIFO to the encoder in chunks,
// defined by the encoder (512 samples by default) or flushes the FIFO if requested.
/// Note: flush might be called only when writing the final data batch, otherwise
// the codec will crash (especially in case of defined size frames like AAC).
/// @param flush Indicates whether to flush the FIFO.
/// @returns 0 on success, -1 or AV_ERROR code on failure
int FFmpegAudioFileWriter::processFifo(bool flush) {
  int result = 0;
  int frameSize = std::max(encoderCtx_->frame_size, fallbackFrameSize);

  while (av_audio_fifo_size(audioFifo_.get()) >= (flush ? 1 : frameSize)) {
    const int chunkSize = std::min(av_audio_fifo_size(audioFifo_.get()), frameSize);

    assert(chunkSize <= writingFrame_->nb_samples);

    if (av_audio_fifo_read(
            audioFifo_.get(), reinterpret_cast<void **>(writingFrame_->data), chunkSize) !=
        chunkSize) {
      invokeOnErrorCallback("Failed to read data from FIFO");
      return -1;
    }

    writingFrame_->nb_samples = chunkSize;
    writingFrame_->pts = nextPts_;
    nextPts_ += chunkSize;

    result = avcodec_send_frame(encoderCtx_.get(), writingFrame_.get());

    if (result < 0) {
      invokeOnErrorCallback("Failed to send frame to encoder: " + parseErrorCode(result));
      return result;
    }

    result = writeEncodedPackets();

    if (result < 0) {
      invokeOnErrorCallback("Failed to write encoded packets: " + parseErrorCode(result));
      return result;
    }
  }

  return 0;
}

/// @brief Takes ready encoded packets from the encoder and writes them to the output file.
/// Also in order to optimize file writing vs file resilience from crashes, it periodically
/// forces the AVIO buffer to flush data to disk, by default every 0,5 second.
/// @returns 0 on success, AV_ERROR code on failure
int FFmpegAudioFileWriter::writeEncodedPackets() {
  int result = 0;

  while (true) {
    result = avcodec_receive_packet(encoderCtx_.get(), packet_.get());

    if (result == AVERROR(EAGAIN) || result == AVERROR_EOF) {
      return 0;
    } else if (result < 0) {
      invokeOnErrorCallback("Failed to receive packet from encoder: " + parseErrorCode(result));
      return result;
    }

    av_packet_rescale_ts(packet_.get(), encoderCtx_->time_base, stream_->time_base);
    packet_->stream_index = stream_->index;

    result = av_interleaved_write_frame(formatCtx_.get(), packet_.get());

    auto now = std::chrono::steady_clock::now();
    auto elapsedMs =
        std::chrono::duration_cast<std::chrono::milliseconds>(now - lastFlushTime_).count();

    if (formatCtx_->pb && elapsedMs >= flushIntervalMs_) {
      avio_flush(formatCtx_->pb);
      lastFlushTime_ = now;
    }

    if (result < 0) {
      return result;
    }
  }
}

/// @brief Closes the currently opened audio file, flushing any remaining data and finalizing the file.
/// Method checks the file size and duration for convenience.
/// @returns CloseFileResult indicating success or error details
CloseFileResult FFmpegAudioFileWriter::finalizeOutput() {
  int result = av_write_trailer(formatCtx_.get());

  if (result < 0) {
    return CloseFileResult::Err("Failed to write trailer: " + parseErrorCode(result));
  }

  double fileSizeInMB = 0;

  if (formatCtx_->pb) {
    fileSizeInMB = static_cast<double>(avio_size(formatCtx_->pb)) / MB_IN_BYTES;
    avio_closep(&formatCtx_->pb);
  }

  double durationInSeconds = getCurrentDuration();

  filePath_ = "";
  isFileOpen_.store(false, std::memory_order_release);

  return CloseFileResult::Ok({fileSizeInMB, durationInSeconds});
}

} // namespace audioapi::android::ffmpeg

#endif // RN_AUDIO_API_FFMPEG_DISABLED
