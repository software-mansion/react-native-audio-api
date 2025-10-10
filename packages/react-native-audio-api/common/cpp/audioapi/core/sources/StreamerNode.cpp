/*
 * This file dynamically links to the FFmpeg library, which is licensed under
 * the GNU Lesser General Public License (LGPL) version 2.1 or later.
 *
 * Our own code in this file is licensed under the MIT License and dynamic
 * linking allows you to use this code without your entire project being subject
 * to the terms of the LGPL. However, note that if you link statically to
 * FFmpeg, you must comply with the terms of the LGPL for FFmpeg itself.
 */

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
      fmtCtx_(nullptr),
      codecCtx_(nullptr),
      decoder_(nullptr),
      codecpar_(nullptr),
      pkt_(nullptr),
      frame_(nullptr),
      pendingFrame_(nullptr),
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
    if (VERBOSE)
      printf("Failed to open input\n");
    return false;
  }

  if (!findAudioStream() || !setupDecoder() || !setupResampler()) {
    if (VERBOSE)
      printf("Failed to find/setup audio stream\n");
    cleanup();
    return false;
  }

  pkt_ = av_packet_alloc();
  frame_ = av_frame_alloc();

  if (pkt_ == nullptr || frame_ == nullptr) {
    if (VERBOSE)
      printf("Failed to allocate packet or frame\n");
    cleanup();
    return false;
  }

  maxBufferSize_ = BUFFER_LENGTH_SECONDS * codecCtx_->sample_rate;
  // If decoding is faster than playing, we buffer few seconds of audio
  bufferedBus_ = std::make_shared<AudioBus>(
      maxBufferSize_, codecpar_->ch_layout.nb_channels, codecCtx_->sample_rate);

  channelCount_ = codecpar_->ch_layout.nb_channels;
  audioBus_ = std::make_shared<AudioBus>(
      RENDER_QUANTUM_SIZE, channelCount_, context_->getSampleRate());

  streamingThread_ = std::thread(&StreamerNode::streamAudio, this);
  streamFlag.store(true);
  isInitialized_ = true;
  return true;
}

void StreamerNode::stop(double when) {
  AudioScheduledSourceNode::stop(when);
  streamFlag.store(false);
}

bool StreamerNode::setupResampler() {
  // Allocate resampler context
  swrCtx_ = swr_alloc();
  if (swrCtx_ == nullptr) {
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
    return false;
  }

  // Allocate output buffer for resampled data
  maxResampledSamples_ = INITIAL_MAX_RESAMPLED_SAMPLES;
  int ret = av_samples_alloc_array_and_samples(
      &resampledData_,
      nullptr,
      codecCtx_->ch_layout.nb_channels,
      maxResampledSamples_,
      AV_SAMPLE_FMT_FLTP,
      0);

  return ret >= 0;
}

void StreamerNode::streamAudio() {
  while (streamFlag.load()) {
    if (pendingFrame_ != nullptr) {
      if (!processFrameWithResampler(pendingFrame_)) {
        return;
      }
    } else {
      if (av_read_frame(fmtCtx_, pkt_) < 0) {
        return;
      }
      if (pkt_->stream_index == audio_stream_index_) {
        if (avcodec_send_packet(codecCtx_, pkt_) != 0) {
          return;
        }
        if (avcodec_receive_frame(codecCtx_, frame_) != 0) {
          return;
        }
        if (!processFrameWithResampler(frame_)) {
          return;
        }
      }
      av_packet_unref(pkt_);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
}

std::shared_ptr<AudioBus> StreamerNode::processNode(
    const std::shared_ptr<AudioBus> &processingBus,
    int framesToProcess) {
  size_t startOffset = 0;
  size_t offsetLength = 0;
  updatePlaybackInfo(processingBus, framesToProcess, startOffset, offsetLength);

  if (!isPlaying() && !isStopScheduled()) {
    processingBus->zero();
    return processingBus;
  }

  // If we have enough buffered data, copy to output bus
  if (bufferedBusIndex_ >= framesToProcess) {
    Locker locker(mutex_);
    for (int ch = 0; ch < processingBus->getNumberOfChannels(); ch++) {
      memcpy(
          processingBus->getChannel(ch)->getData(),
          bufferedBus_->getChannel(ch)->getData(),
          offsetLength * sizeof(float));

      memmove(
          bufferedBus_->getChannel(ch)->getData(),
          bufferedBus_->getChannel(ch)->getData() + offsetLength,
          (maxBufferSize_ - offsetLength) * sizeof(float));
    }
    bufferedBusIndex_ -= offsetLength;
  } else {
    if (VERBOSE)
      printf(
          "Buffer underrun: have %zu, need %zu\n",
          bufferedBusIndex_,
          (size_t)framesToProcess);
    processingBus->zero();
  }

  return processingBus;
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
    return false;
  }

  // Check if converted data fits in buffer
  if (bufferedBusIndex_ + converted_samples > maxBufferSize_) {
    pendingFrame_ = frame;
    return true;
  } else {
    pendingFrame_ = nullptr;
  }

  // Copy converted data to our buffer
  Locker locker(mutex_);
  for (int ch = 0; ch < codecCtx_->ch_layout.nb_channels; ch++) {
    auto *src = reinterpret_cast<float *>(resampledData_[ch]);
    float *dst = bufferedBus_->getChannel(ch)->getData() + bufferedBusIndex_;
    memcpy(dst, src, converted_samples * sizeof(float));
  }
  bufferedBusIndex_ += converted_samples;
  return true;
}

bool StreamerNode::openInput(const std::string &input_url) {
  if (avformat_open_input(&fmtCtx_, input_url.c_str(), nullptr, nullptr) < 0) {
    return false;
  }
  return avformat_find_stream_info(fmtCtx_, nullptr) >= 0;
}

bool StreamerNode::findAudioStream() {
  audio_stream_index_ = -1;
  codecpar_ = nullptr;

  for (int i = 0; i < fmtCtx_->nb_streams; ++i) {
    if (fmtCtx_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
      audio_stream_index_ = i;
      codecpar_ = fmtCtx_->streams[i]->codecpar;
      break;
    }
  }

  return audio_stream_index_ >= 0 && codecpar_ != nullptr;
}

bool StreamerNode::setupDecoder() {
  decoder_ = avcodec_find_decoder(codecpar_->codec_id);
  if (decoder_ == nullptr) {
    return false;
  }

  codecCtx_ = avcodec_alloc_context3(decoder_);
  if (codecCtx_ == nullptr) {
    return false;
  }

  if (avcodec_parameters_to_context(codecCtx_, codecpar_) < 0) {
    return false;
  }

  return avcodec_open2(codecCtx_, decoder_, nullptr) >= 0;
}

void StreamerNode::cleanup() {
  streamFlag.store(false);
  // cleanup cannot be called from the streaming thread so there is no need to
  // check if we are in the same thread
  streamingThread_.join();
  if (swrCtx_ != nullptr) {
    swr_free(&swrCtx_);
  }

  if (resampledData_ != nullptr) {
    av_freep(&resampledData_[0]);
    av_freep(&resampledData_);
  }

  if (frame_ != nullptr) {
    av_frame_free(&frame_);
  }

  if (pkt_ != nullptr) {
    av_packet_free(&pkt_);
  }

  if (codecCtx_ != nullptr) {
    avcodec_free_context(&codecCtx_);
  }

  if (fmtCtx_ != nullptr) {
    avformat_close_input(&fmtCtx_);
  }

  audio_stream_index_ = -1;
  isInitialized_ = false;
  decoder_ = nullptr;
  codecpar_ = nullptr;
  maxResampledSamples_ = 0;
}
} // namespace audioapi
