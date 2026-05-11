#pragma once

#pragma once

#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/sources/AudioScheduledSourceNode.h>
#include <audioapi/core/utils/Locker.h>
#include <audioapi/types/NodeOptions.h>
#include <audioapi/utils/AudioArray.hpp>
#include <audioapi/utils/AudioBuffer.hpp>

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

#include <audioapi/dsp/r8brain/Resampler.hpp>
#include <audioapi/libs/decoding/IncrementalAudioDecoder.h>
#include <audioapi/utils/SpscChannel.hpp>
#include <atomic>
#include <memory>
#include <string>
#include <utility>

// TODO: change names, prob move to constants.h (?)
inline constexpr auto STREAMER_NODE_SPSC_OVERFLOW_STRATEGY =
    audioapi::channels::spsc::OverflowStrategy::WAIT_ON_FULL;
inline constexpr auto STREAMER_NODE_SPSC_WAIT_STRATEGY =
    audioapi::channels::spsc::WaitStrategy::ATOMIC_WAIT;

inline constexpr auto VERBOSE = false;
inline constexpr auto CHANNEL_CAPACITY = 32;

// TODO: check if copy constructors are needed
struct StreamingData {
  audioapi::AudioBuffer buffer;
  size_t size{0};

  StreamingData() = default;
  ~StreamingData() = default;
  StreamingData(audioapi::AudioBuffer b, size_t s) : buffer(std::move(b)), size(s) {}
  StreamingData(const StreamingData &data) = default;
  StreamingData(StreamingData &&data) noexcept : buffer(std::move(data.buffer)), size(data.size) {}
  StreamingData &operator=(StreamingData &&other) = default;
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

class HLSDecoder : public decoding::IIncrementalAudioDecoder {
  /// @brief Opens a file for decoding.
  /// @param outputSampleRate The output sample rate.
  /// @param path The path to the file.
  /// @return True if the file was opened successfully, false otherwise.
  [[nodiscard]] bool openFile(int outputSampleRate, const std::string &path) override;

  /// @brief Opens a memory block for decoding.
  /// @param outputSampleRate The output sample rate.
  /// @param data The data to decode.
  /// @param size The size of the data.
  /// @return True if the memory block was opened successfully, false otherwise.
  [[nodiscard]] bool openMemory(int outputSampleRate, const void *data, size_t size) override;

  /// @brief Reads PCM frames from the decoder.
  /// @param outInterleaved The output buffer for the decoded frames.
  /// @param frameCount The number of frames to read.
  /// @return The number of frames read.
  [[nodiscard]] size_t readPcmFrames(float *outInterleaved, size_t frameCount) override;

  /// @brief Closes the decoder.
  void close() override;

  /// @brief Checks if the decoder is open.
  /// @return True if the decoder is open, false otherwise.
  [[nodiscard]] bool isOpen() const override;

  /// @brief Gets the number of output channels.
  /// @return The number of output channels.
  [[nodiscard]] int outputChannels() const override;

  /// @brief Gets the output sample rate.
  /// @return The output sample rate.
  [[nodiscard]] int outputSampleRate() const override;

  /// @brief Gets the duration of the audio in seconds.
  /// @return The duration of the audio in seconds.
  [[nodiscard]] float getDurationInSeconds() const override;

  /// @brief Gets the current position of the audio in seconds.
  /// @return The current position of the audio in seconds.
  [[nodiscard]] float getCurrentPositionInSeconds() const override;

  /// @brief Seeks to a specific time in the audio.
  /// @param seconds The time to seek to in seconds.
  /// @return True if the seek was successful, false otherwise.
  [[nodiscard]] bool seekToTime(double seconds) override;

#if !RN_AUDIO_API_FFMPEG_DISABLED
  AVFormatContext *fmtCtx_;
  AVCodecContext *codecCtx_;
  const AVCodec *decoder_;
  AVCodecParameters *codecpar_;
  AVPacket *pkt_;
  AVFrame *frame_; // Frame that is currently being processed
  SwrContext *swrCtx_;

  // --resampling--
  AudioBuffer resamplerInputBuffer_;
  AudioBuffer resamplerOutputBuffer_;
  StreamingData bufferedAudioData_; // audio data for buffering hls frames
  bool hasBufferedAudioData_;
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

  /// @brief Initialize the StreamerNode by opening the input stream,
  /// finding the audio stream, setting up the decoder, and starting the streaming thread.
  /// @param inputUrl The URL of the input stream
  /// @return true if initialization was successful, false otherwise
  bool initialize(const std::string &inputUrl);

  /**
   * @brief Setting up the resampler
   * @param outSampleRate Sample rate for the output audio
   * @return true if successful, false otherwise
   */
  bool setupResampler(float outSampleRate);

  /**
   * @brief Resample the audio frame, change its sample format and channel layout
   * @param frame The AVFrame to resample
   * @param context The context
   */
  void processFrameWithResampler(AVFrame *frame, const std::shared_ptr<BaseAudioContext> &context);

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