#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/sources/StreamerNode.h>
#include <audioapi/core/utils/Locker.h>
#include <audioapi/dsp/VectorMath.h>
#include <audioapi/utils/AudioArray.h>
#include <audioapi/utils/AudioBus.h>
#include <chrono>

namespace audioapi {
StreamerNode::StreamerNode(BaseAudioContext *context)
    : AudioScheduledSourceNode(context),
      streamPath_(""),
      fmtCtx_(nullptr),
      codecCtx_(nullptr),
      decoder_(nullptr),
      codecpar_(nullptr),
      pkt_(nullptr),
      frame_(nullptr),
      pendingFrame_(nullptr),
      hasPendingFrame_(false),
      bufferedBus_(nullptr),
      bufferedBusIndex_(0),
      maxBufferSize_(0),
      audio_stream_index_(-1),
      swrCtx_(nullptr),
      resampledData_(nullptr),
      maxResampledSamples_(0) {}

StreamerNode::~StreamerNode() {
  cleanup();
}

bool StreamerNode::initialize(const std::string &input_url) {
  if (isInitialized_) {
    cleanup();
  }

  if (!openInput(input_url)) {
    return false;
  }

  if (!findAudioStream()) {
    cleanup();
    return false;
  }

  if (!setupDecoder()) {
    cleanup();
    return false;
  }

  if (!setupResampler()) {
    cleanup();
    return false;
  }

  pkt_ = av_packet_alloc();
  frame_ = av_frame_alloc();

  if (!pkt_ || !frame_) {
    std::cerr << "Could not allocate packet or frame" << std::endl;
    cleanup();
    return false;
  }

  maxBufferSize_ = 5 * codecCtx_->sample_rate;
  // if decoding is faster than playing, we buffer 5 seconds of audio
  bufferedBus_ = std::make_shared<AudioBus>(
      maxBufferSize_, codecpar_->ch_layout.nb_channels, codecCtx_->sample_rate);

  thread_ = std::thread(&StreamerNode::streamAudio, this);
  streamFlag.store(true);
  isInitialized_ = true;
  return true;
}

bool StreamerNode::setupResampler() {
  // Allocate resampler context
  swrCtx_ = swr_alloc();
  if (!swrCtx_) {
    std::cerr << "Could not allocate resampler context" << std::endl;
    return false;
  }

  // Set input parameters (from codec)
  av_opt_set_chlayout(swrCtx_, "in_chlayout", &codecCtx_->ch_layout, 0);
  av_opt_set_int(swrCtx_, "in_sample_rate", codecCtx_->sample_rate, 0);
  av_opt_set_sample_fmt(swrCtx_, "in_sample_fmt", codecCtx_->sample_fmt, 0);

  // Set output parameters (float)
  av_opt_set_chlayout(swrCtx_, "out_chlayout", &codecCtx_->ch_layout, 0);
  av_opt_set_int(swrCtx_, "out_sample_rate", context_->getSampleRate(), 0);
  av_opt_set_sample_fmt(swrCtx_, "out_sample_fmt", AV_SAMPLE_FMT_FLTP, 0);

  // Initialize the resampler
  if (swr_init(swrCtx_) < 0) {
    std::cerr << "Could not initialize resampler" << std::endl;
    return false;
  }

  // Allocate output buffer for resampled data
  maxResampledSamples_ = 8192; // Start with reasonable buffer size
  int ret = av_samples_alloc_array_and_samples(
      &resampledData_,
      nullptr,
      codecCtx_->ch_layout.nb_channels,
      maxResampledSamples_,
      AV_SAMPLE_FMT_FLTP,
      0);

  if (ret < 0) {
    std::cerr << "Could not allocate resampled data buffer" << std::endl;
    return false;
  }

  return true;
}

void StreamerNode::streamAudio() {
  while (streamFlag.load()) {
    if (pendingFrame_) {
      if (!processFrameWithResampler(pendingFrame_)) {
        std::cerr << "Error processing pending frame" << std::endl;
        cleanup();
        return;
      }
    } else {
      if (av_read_frame(fmtCtx_, pkt_) >= 0) {
        if (pkt_->stream_index == audio_stream_index_) {
          if (avcodec_send_packet(codecCtx_, pkt_) == 0) {
            if (avcodec_receive_frame(codecCtx_, frame_) == 0) {
              if (!processFrameWithResampler(frame_)) {
                std::cerr << "Error processing frame with resampler"
                          << std::endl;
                cleanup();
                return;
              }
            } else {
              std::cerr << "Error receiving frame from codec" << std::endl;
              cleanup();
              return;
            }
          } else {
            std::cerr << "Error decoding audio packet" << std::endl;
            cleanup();
            return;
          }
        }
        av_packet_unref(pkt_);
      } else {
        std::cerr << "Error reading frame from input stream" << std::endl;
        cleanup();
        return;
      }
    }
  }
}

void StreamerNode::processNode(
    const std::shared_ptr<AudioBus> &processingBus,
    int framesToProcess) {
  // TODO: BETTER HANDLING STOPPING
  if (playbackState_ == PlaybackState::FINISHED) {
    return;
  }
  // If we have enough buffered data, copy to output bus
  if (bufferedBusIndex_ >= framesToProcess) {
    Locker locker(mutex_);
    for (int ch = 0; ch < processingBus->getNumberOfChannels(); ch++) {
      memcpy(
          processingBus->getChannel(ch)->getData(),
          bufferedBus_->getChannel(ch)->getData(),
          framesToProcess * sizeof(float));

      memmove(
          bufferedBus_->getChannel(ch)->getData(),
          bufferedBus_->getChannel(ch)->getData() + framesToProcess,
          (maxBufferSize_ - framesToProcess) * sizeof(float));
    }
    bufferedBusIndex_ -= framesToProcess;
  }
}

bool StreamerNode::processFrameWithResampler(AVFrame *frame) {
  // Check if we need to reallocate the resampled buffer
  int out_samples = swr_get_out_samples(swrCtx_, frame->nb_samples);
  if (out_samples > maxResampledSamples_) {
    av_freep(&resampledData_[0]);
    av_freep(&resampledData_);

    maxResampledSamples_ = out_samples;
    int ret = av_samples_alloc_array_and_samples(
        &resampledData_,
        nullptr,
        codecCtx_->ch_layout.nb_channels,
        maxResampledSamples_,
        AV_SAMPLE_FMT_FLTP,
        0);

    if (ret < 0) {
      std::cerr << "Could not reallocate resampled data buffer" << std::endl;
      return false;
    }
  }

  // Convert the frame
  int converted_samples = swr_convert(
      swrCtx_,
      resampledData_,
      maxResampledSamples_,
      (const uint8_t **)frame->data,
      frame->nb_samples);

  if (converted_samples < 0) {
    std::cerr << "Error converting samples" << std::endl;
    return false;
  }

  // Check if converted data fits in buffer
  if (bufferedBusIndex_ + converted_samples > maxBufferSize_) {
    hasPendingFrame_ = true;
    pendingFrame_ = frame;
    return true;
  } else {
    hasPendingFrame_ = false;
    pendingFrame_ = nullptr;
  }

  // Copy converted data to our buffer
  Locker locker(mutex_);
  for (int ch = 0; ch < codecCtx_->ch_layout.nb_channels; ch++) {
    float *src = reinterpret_cast<float *>(resampledData_[ch]);
    float *dst = bufferedBus_->getChannel(ch)->getData() + bufferedBusIndex_;
    memcpy(dst, src, converted_samples * sizeof(float));
  }
  bufferedBusIndex_ += converted_samples;
  return true;
}

bool StreamerNode::openInput(const std::string &input_url) {
  int ret = avformat_open_input(&fmtCtx_, input_url.c_str(), nullptr, nullptr);
  if (ret < 0) {
    char errbuf[256];
    av_strerror(ret, errbuf, sizeof(errbuf));
    std::cerr << "Could not open input: " << input_url << "\nReason: " << errbuf
              << std::endl;
    return false;
  }

  if (avformat_find_stream_info(fmtCtx_, nullptr) < 0) {
    std::cerr << "Failed to retrieve input stream information" << std::endl;
    return false;
  }

  return true;
}

bool StreamerNode::findAudioStream() {
  audio_stream_index_ = -1;
  codecpar_ = nullptr;

  for (unsigned int i = 0; i < fmtCtx_->nb_streams; ++i) {
    if (fmtCtx_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
      audio_stream_index_ = i;
      codecpar_ = fmtCtx_->streams[i]->codecpar;
      break;
    }
  }

  if (audio_stream_index_ < 0 || !codecpar_) {
    std::cerr << "Could not find audio stream in input" << std::endl;
    return false;
  }

  return true;
}

bool StreamerNode::setupDecoder() {
  decoder_ = avcodec_find_decoder(codecpar_->codec_id);
  if (!decoder_) {
    std::cerr << "Could not find decoder for codec ID: " << codecpar_->codec_id
              << std::endl;
    return false;
  }

  codecCtx_ = avcodec_alloc_context3(decoder_);
  if (!codecCtx_) {
    std::cerr << "Could not allocate codec context" << std::endl;
    return false;
  }

  if (avcodec_parameters_to_context(codecCtx_, codecpar_) < 0) {
    std::cerr << "Could not copy codec parameters to context" << std::endl;
    return false;
  }

  if (avcodec_open2(codecCtx_, decoder_, nullptr) < 0) {
    std::cerr << "Could not open codec" << std::endl;
    return false;
  }

  return true;
}

void StreamerNode::cleanup() {
  // Clean up resampler
  streamFlag.store(false);
  thread_.join();
  if (swrCtx_) {
    swr_free(&swrCtx_);
  }

  if (resampledData_) {
    av_freep(&resampledData_[0]);
    av_freep(&resampledData_);
  }

  if (frame_) {
    av_frame_free(&frame_);
  }

  if (pkt_) {
    av_packet_free(&pkt_);
  }

  if (codecCtx_) {
    avcodec_free_context(&codecCtx_);
  }

  if (fmtCtx_) {
    avformat_close_input(&fmtCtx_);
  }

  audio_stream_index_ = -1;
  isInitialized_ = false;
  decoder_ = nullptr;
  codecpar_ = nullptr;
  maxResampledSamples_ = 0;
}

void StreamerNode::startStreaming() {
  if (!isInitialized_) {
    std::cerr << "StreamerNode is not initialized" << std::endl;
    return;
  }

  if (playbackState_ == PlaybackState::PLAYING) {
    std::cerr << "StreamerNode is already playing" << std::endl;
    return;
  }
}

void StreamerNode::stopStreaming() {
  if (playbackState_ == PlaybackState::UNSCHEDULED ||
      playbackState_ == PlaybackState::FINISHED) {
    std::cerr << "StreamerNode is not playing" << std::endl;
    return;
  }
  playbackState_ = PlaybackState::FINISHED;

  cleanup();
}
} // namespace audioapi
