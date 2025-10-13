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

#ifndef AUDIO_API_TEST_SUITE
extern "C" {
  #include <libavformat/avformat.h>
  #include <libavcodec/avcodec.h>
  #include <libavutil/samplefmt.h>
  #include <libavutil/channel_layout.h>
  #include <libavutil/opt.h>
  #include <libswresample/swresample.h>
}
#endif

#include <cmath>
#include <memory>
#include <string>
#include <atomic>

static bool constexpr VERBOSE = false;

namespace audioapi {

class AudioBus;

class StreamerNode : public AudioScheduledSourceNode {
 public:
  explicit StreamerNode(BaseAudioContext *context);
  ~StreamerNode() override;

  /**
   * @brief Initialize all necessary ffmpeg components for streaming audio
  */
  bool initialize(const std::string& inputUrl);
  void stop(double when) override;

 protected:
  std::shared_ptr<AudioBus> processNode(const std::shared_ptr<AudioBus>& processingBus, int framesToProcess) override;

 private:
  #ifndef AUDIO_API_TEST_SUITE
  AVFormatContext* fmtCtx_;
  AVCodecContext* codecCtx_;
  const AVCodec* decoder_;
  AVCodecParameters* codecpar_;
  AVPacket* pkt_;
  AVFrame* frame_; // Frame that is currently being processed
  AVFrame* pendingFrame_; // Frame that is saved if bufferedBus is full
  std::shared_ptr<AudioBus> bufferedBus_; // audio bus for buffering hls frames
  size_t bufferedBusIndex_; // index in the buffered bus where we write the next frame
  size_t maxBufferSize_; // maximum size of the buffered bus
  int audio_stream_index_; // index of the audio stream channel in the input
  SwrContext* swrCtx_;
  uint8_t** resampledData_; // weird ffmpeg way of using raw byte pointers for resampled data
  int maxResampledSamples_;
  std::mutex mutex_;
  std::thread streamingThread_;
  std::atomic<bool> streamFlag; // Flag to control the streaming thread
  static constexpr float BUFFER_LENGTH_SECONDS = 5.0f; // Length of the buffer in seconds
  static constexpr int INITIAL_MAX_RESAMPLED_SAMPLES = 8192; // Initial size for resampled data

  /**
   * @brief Setting up the resampler
   * @return true if successful, false otherwise
   */
  bool setupResampler();

  /**
   * @brief Resample the audio frame, change its sample format and channel layout
   * @param frame The AVFrame to resample
   * @return true if successful, false otherwise
   */
  bool processFrameWithResampler(AVFrame* frame);

  /**
   * @brief Thread function to continuously read and process audio frames
   * @details This function runs in a separate thread to avoid blocking the main audio processing thread
   * @note It will read frames from the input stream, resample them, and store them in the buffered bus
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
  bool openInput(const std::string& inputUrl);

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
  #endif // AUDIO_API_TEST_SUITE
};
} // namespace audioapi
