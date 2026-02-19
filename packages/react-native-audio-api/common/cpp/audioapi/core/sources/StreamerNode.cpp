/*
 * This file dynamically links to the FFmpeg library, which is licensed under
 * the GNU Lesser General Public License (LGPL) version 2.1 or later.
 *
 * Our own code in this file is licensed under the MIT License and dynamic
 * linking allows you to use this code without your entire project being subject
 * to the terms of the LGPL. However, note that if you link statically to
 * FFmpeg, you must comply with the terms of the LGPL for FFmpeg itself.
 */

#include <audioapi/types/NodeOptions.h>
#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/sources/StreamerNode.h>
#include <audioapi/core/utils/Locker.h>
#include <audioapi/utils/AudioArray.h>
#include <audioapi/utils/AudioBuffer.h>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace audioapi {
#if !RN_AUDIO_API_FFMPEG_DISABLED
StreamerNode::StreamerNode(
    const std::shared_ptr<BaseAudioContext> &context,
    const StreamerOptions &options)
    : AudioScheduledSourceNode(context, options),
      fmtCtx_(nullptr),
      codecCtx_(nullptr),
      decoder_(nullptr),
      codecpar_(nullptr),
      pkt_(nullptr),
      frame_(nullptr),
      swrCtx_(nullptr),
      resampledData_(nullptr),
      bufferedAudioBuffer_(nullptr),
      bufferedAudioBufferSize_(0),
      audio_stream_index_(-1),
      maxResampledSamples_(0),
      processedSamples_(0) {}
#else
StreamerNode::StreamerNode(
    const std::shared_ptr<BaseAudioContext> &context,
    const StreamerOptions &options)
    : AudioScheduledSourceNode(context) {}
#endif // RN_AUDIO_API_FFMPEG_DISABLED

StreamerNode::~StreamerNode() {
#if !RN_AUDIO_API_FFMPEG_DISABLED
  cleanup();
#endif // RN_AUDIO_API_FFMPEG_DISABLED
}

bool StreamerNode::initialize(const std::string &input_url) {
#if !RN_AUDIO_API_FFMPEG_DISABLED
  streamPath_ = input_url;
  std::shared_ptr<BaseAudioContext> context = context_.lock();
  if (context == nullptr) {
    return false;
  }

  if (isInitialized_) {
    cleanup();
  }

  if (!openInput(input_url)) {
    if (VERBOSE)
      printf("Failed to open input\n");
    return false;
  }

  if (!findAudioStream() || !setupDecoder() || !setupResampler(context->getSampleRate())) {
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

  channelCount_ = codecpar_->ch_layout.nb_channels;
  audioBuffer_ =
      std::make_shared<AudioBuffer>(RENDER_QUANTUM_SIZE, channelCount_, context->getSampleRate());

  auto [sender, receiver] = channels::spsc::channel<
      StreamingData,
      STREAMER_NODE_SPSC_OVERFLOW_STRATEGY,
      STREAMER_NODE_SPSC_WAIT_STRATEGY>(CHANNEL_CAPACITY);
  sender_ = std::move(sender);
  receiver_ = std::move(receiver);

  streamingThread_ = std::thread(&StreamerNode::streamAudio, this);
  isInitialized_ = true;
  return true;
#else
  return false;
#endif // RN_AUDIO_API_FFMPEG_DISABLED
}

std::shared_ptr<AudioBuffer> StreamerNode::processNode(
    const std::shared_ptr<AudioBuffer> &processingBuffer,
    int framesToProcess) {
#if !RN_AUDIO_API_FFMPEG_DISABLED
  size_t startOffset = 0;
  size_t offsetLength = 0;
  std::shared_ptr<BaseAudioContext> context = context_.lock();
  if (context == nullptr) {
    processingBuffer->zero();
    return processingBuffer;
  }
  updatePlaybackInfo(
      processingBuffer,
      framesToProcess,
      startOffset,
      offsetLength,
      context->getSampleRate(),
      context->getCurrentSampleFrame());
  isNodeFinished_.store(isFinished(), std::memory_order_release);

  if (!isPlaying() && !isStopScheduled()) {
    processingBuffer->zero();
    return processingBuffer;
  }

  int bufferRemaining = bufferedAudioBufferSize_ - processedSamples_;
  int alreadyProcessed = 0;
  if (bufferRemaining < framesToProcess) {
    if (bufferedAudioBuffer_ != nullptr) {
      processingBuffer->copy(*bufferedAudioBuffer_, processedSamples_, 0, bufferRemaining);
      framesToProcess -= bufferRemaining;
      alreadyProcessed += bufferRemaining;
    }
    StreamingData data;
    auto res = receiver_.try_receive(data);
    if (res == channels::spsc::ResponseStatus::SUCCESS) {
      bufferedAudioBuffer_ = std::make_shared<AudioBuffer>(std::move(data.buffer));
      bufferedAudioBufferSize_ = data.size;
      processedSamples_ = 0;
    } else {
      bufferedAudioBuffer_ = nullptr;
    }
  }
  if (bufferedAudioBuffer_ != nullptr) {
    processingBuffer->copy(*bufferedAudioBuffer_, processedSamples_, alreadyProcessed, framesToProcess);
    processedSamples_ += framesToProcess;
  }
#endif // RN_AUDIO_API_FFMPEG_DISABLED

  return processingBuffer;
}

#if !RN_AUDIO_API_FFMPEG_DISABLED
bool StreamerNode::setupResampler(float outSampleRate) {
  int n = codecCtx_->ch_layout.nb_channels;
  resampler_ = std::make_unique<r8b::MultiChannelResampler>(codecCtx_->sample_rate, outSampleRate, n);
  outSampleRate_ = outSampleRate;
  return true;
}

void StreamerNode::streamAudio() {
  while (!isNodeFinished_.load(std::memory_order_acquire)) {
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
      std::shared_ptr<BaseAudioContext> context = context_.lock();
      if (context == nullptr) {
        return;
      }
      if (!processFrameWithResampler(frame_, context)) {
        return;
      }
    }
    av_packet_unref(pkt_);
  }
}

static void extractChannelAsFloat(
    const AVFrame *frame,
    int channel,
    float *output) {
  const int nb = frame->nb_samples;

  switch (frame->format) {
    case AV_SAMPLE_FMT_FLTP: {
      std::memcpy(
          output,
          reinterpret_cast<const float *>(frame->data[channel]),
          nb * sizeof(float));
      break;
    }
    case AV_SAMPLE_FMT_DBLP: {
      auto *src = reinterpret_cast<const double *>(frame->data[channel]);
      for (int i = 0; i < nb; ++i) {
        output[i] = static_cast<float>(src[i]);
      }
      break;
    }
    case AV_SAMPLE_FMT_S16P: {
      auto *src = reinterpret_cast<const int16_t *>(frame->data[channel]);
      for (int i = 0; i < nb; ++i) {
        output[i] = src[i] / 32768.0f;
      }
      break;
    }
    case AV_SAMPLE_FMT_S32P: {
      auto *src = reinterpret_cast<const int32_t *>(frame->data[channel]);
      for (int i = 0; i < nb; ++i) {
        output[i] = src[i] / 2147483648.0f;
      }
      break;
    }
    case AV_SAMPLE_FMT_U8P: {
      auto *src = frame->data[channel];
      for (int i = 0; i < nb; ++i) {
        output[i] = (src[i] - 128) / 128.0f;
      }
      break;
    }
    default:
      std::memset(output, 0, nb * sizeof(float));
      break;
  }
}

bool StreamerNode::processFrameWithResampler(
    AVFrame *frame,
    const std::shared_ptr<BaseAudioContext> &context) {
  if (this->isFinished()) {
    return true;
  }

  const int numChannels = frame->ch_layout.nb_channels;
  const int nbSamples = frame->nb_samples;
  const bool needsResample =
      static_cast<int>(outSampleRate_) != frame->sample_rate;

  std::vector<std::vector<float>> inputBuffers(
      numChannels, std::vector<float>(nbSamples));
  for (int ch = 0; ch < numChannels; ++ch) {
    extractChannelAsFloat(frame, ch, inputBuffers[ch].data());
  }

  int outSamples;
  std::vector<float *> copyVector(numChannels, nullptr);

  if (needsResample) {
    std::vector<float *> inputPtrs(numChannels);
    for (int ch = 0; ch < numChannels; ++ch) {
      inputPtrs[ch] = inputBuffers[ch].data();
    }

    outSamples = resampler_->process(inputPtrs, nbSamples, copyVector);
  } else {
    outSamples = nbSamples;
    for (int ch = 0; ch < numChannels; ++ch) {
      copyVector[ch] = inputBuffers[ch].data();
    }
  }

  auto buffer =
  AudioBuffer(outSamples, numChannels, context->getSampleRate());
  for (int ch = 0; ch < numChannels; ++ch) {
    buffer.getChannel(ch)->copy(copyVector[ch], 0, 0, outSamples);
  }

  StreamingData data{std::move(buffer), static_cast<size_t>(outSamples)};
  sender_.send(std::move(data));

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
  this->playbackState_ = PlaybackState::FINISHED;
  isNodeFinished_.store(true, std::memory_order_release);
  if (streamingThread_.joinable()) {
    StreamingData dummy;
    while (receiver_.try_receive(dummy) == channels::spsc::ResponseStatus::SUCCESS)
      ; // clear the receiver
    streamingThread_.join();
  }
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

  resampler_.reset();
  audio_stream_index_ = -1;
  isInitialized_ = false;
  decoder_ = nullptr;
  codecpar_ = nullptr;
  maxResampledSamples_ = 0;
}
#endif // RN_AUDIO_API_FFMPEG_DISABLED
} // namespace audioapi
