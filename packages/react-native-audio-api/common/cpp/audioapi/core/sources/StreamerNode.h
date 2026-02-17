/*
 * This file dynamically links to the FFmpeg library, which is licensed under the
 * GNU Lesser General Public License (LGPL) version 2.1 or later.
 *
 * Our own code in this file is licensed under the MIT License and dynamic linking
 * allows you to use this code without your entire project being subject to the
 * terms of the LGPL. However, note that if you link statically to FFmpeg, you must
 * comply with the terms of the LGPL for FFmpeg itself.
 */

#pragma once

#include <audioapi/core/sources/AudioScheduledSourceNode.h>
#include <audioapi/utils/AudioBuffer.h>

#if !RN_AUDIO_API_FFMPEG_DISABLED
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
#include <libavutil/samplefmt.h>
#include <libswresample/swresample.h>
}
#endif // RN_AUDIO_API_FFMPEG_DISABLED

#include <audioapi/utils/SpscChannel.hpp>
#include <atomic>
#include <cmath>
#include <memory>
#include <string>
#include <utility>

inline constexpr auto STREAMER_NODE_SPSC_OVERFLOW_STRATEGY =
    audioapi::channels::spsc::OverflowStrategy::WAIT_ON_FULL;
inline constexpr auto STREAMER_NODE_SPSC_WAIT_STRATEGY =
    audioapi::channels::spsc::WaitStrategy::ATOMIC_WAIT;

inline constexpr auto VERBOSE = false;
inline constexpr auto CHANNEL_CAPACITY = 32;

struct StreamingData {
  audioapi::AudioBuffer buffer;
  size_t size;
  StreamingData() = default;
  StreamingData(audioapi::AudioBuffer b, size_t s) : buffer(b), size(s) {}
  StreamingData(const StreamingData &data) : buffer(data.buffer), size(data.size) {}
  StreamingData(StreamingData &&data) noexcept : buffer(std::move(data.buffer)), size(data.size) {}
  StreamingData &operator=(const StreamingData &data) {
    if (this == &data) {
      return *this;
    }
    buffer = data.buffer;
    size = data.size;
    return *this;
  }
};

namespace audioapi {

class AudioBuffer;
struct StreamerOptions;

class StreamerNode : public AudioScheduledSourceNode {
 public:
  explicit StreamerNode(
      const std::shared_ptr<BaseAudioContext> &context,
      const StreamerOptions &options);
  ~StreamerNode() override;

  /**
   * @brief Initialize all necessary ffmpeg components for streaming audio
  */
  bool initialize(const std::string &inputUrl);

  std::string getStreamPath() const {
    return streamPath_;
  }

 protected:
  std::shared_ptr<AudioBuffer> processNode(
      const std::shared_ptr<AudioBuffer> &processingBuffer,
      int framesToProcess) override;

 private:
  std::string streamPath_;

#if !RN_AUDIO_API_FFMPEG_DISABLED
  AVFormatContext *fmtCtx_;
  AVCodecContext *codecCtx_;
  const AVCodec *decoder_;
  AVCodecParameters *codecpar_;
  AVPacket *pkt_;
  AVFrame *frame_; // Frame that is currently being processed
  SwrContext *swrCtx_;
  uint8_t **resampledData_; // weird ffmpeg way of using raw byte pointers for resampled data

  std::shared_ptr<AudioBuffer> bufferedAudioBuffer_; // audio buffer for buffering hls frames
  size_t bufferedAudioBufferSize_;                   // size of currently buffered buffer
  int audio_stream_index_; // index of the audio stream channel in the input
  int maxResampledSamples_;
  size_t processedSamples_;

  std::thread streamingThread_;
  std::atomic<bool> isNodeFinished_;                         // Flag to control the streaming thread
  static constexpr int INITIAL_MAX_RESAMPLED_SAMPLES = 8192; // Initial size for resampled data
  channels::spsc::
      Sender<StreamingData, STREAMER_NODE_SPSC_OVERFLOW_STRATEGY, STREAMER_NODE_SPSC_WAIT_STRATEGY>
          sender_;
  channels::spsc::Receiver<
      StreamingData,
      STREAMER_NODE_SPSC_OVERFLOW_STRATEGY,
      STREAMER_NODE_SPSC_WAIT_STRATEGY>
      receiver_;

  /**
   * @brief Setting up the resampler
   * @param outSampleRate Sample rate for the output audio
   * @return true if successful, false otherwise
   */
  bool setupResampler(float outSampleRate);

  /**
   * @brief Resample the audio frame, change its sample format and channel layout
   * @param frame The AVFrame to resample
   * @return true if successful, false otherwise
   */
  bool processFrameWithResampler(AVFrame *frame, const std::shared_ptr<BaseAudioContext> &context);

  /**
   * @brief Thread function to continuously read and process audio frames
   * @details This function runs in a separate thread to avoid blocking the main audio processing thread
   * @note It will read frames from the input stream, resample them, and store them in the buffered buffer
   * @note The thread will stop when streamFlag is set to false
   */
  void streamAudio();

  /** @brief Clean up resources */
  void cleanup();

  /**
   * @brief Open the input stream
   * @param inputUrl The URL of the input stream
   * @return true if successful, false otherwise
   * @note This function initializes the FFmpeg libraries and opens the input stream
   */
  bool openInput(const std::string &inputUrl);

  /**
   * @brief Find the audio stream channel in the input
   * @return true if audio stream was found, false otherwise
   */
  bool findAudioStream();

  /**
   * @brief Set up the decoder for the audio stream
   * @return true if successful, false otherwise
   */
  bool setupDecoder();
#endif // RN_AUDIO_API_FFMPEG_DISABLED
};
} // namespace audioapi
