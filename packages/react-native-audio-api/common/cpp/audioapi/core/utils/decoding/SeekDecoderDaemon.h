#pragma once

#include <audioapi/core/utils/Constants.h>
#include <audioapi/libs/decoding/IncrementalAudioDecoder.h>
#if !RN_AUDIO_API_FFMPEG_DISABLED
#include <audioapi/libs/ffmpeg/FFmpegDecoding.h>
#endif // RN_AUDIO_API_FFMPEG_DISABLED
#include <audioapi/libs/miniaudio/MiniAudioDecoding.h>
#include <audioapi/utils/SpscChannel.hpp>
#include <array>
#include <memory>
#include <optional>
#include <string>
#include <vector>

using namespace audioapi::channels::spsc;

struct SeekDecoderDaemonOptions {
  bool requiresFFmpeg;
  // source
  std::string filePath;
  std::vector<uint8_t> memoryData;
  // playback
  float contextSampleRate;
  bool loop;
};

struct AudioFileDecoderState {
  // Lifecycle and Sync
  std::atomic<bool> isDaemonRunning{true};
  std::atomic<bool> isReady{false}; // True once the decoder opens the file/URL
  std::atomic<int> pendingOffloadedSeeks{0};

  // Metadata
  std::atomic<int> channelCount{0};
  std::atomic<float> sampleRate{0.0f};
  std::atomic<double> duration{0.0};

  // Playback state
  std::atomic<double> currentTime{0.0};
  std::atomic<bool> loop{false};

  /// True when the opened source is an FFmpeg HLS live/indefinite stream.
  std::atomic<bool> isHlsStreaming{false};
};

struct SeekRequest {
  double seconds = 0;
  SeekRequest() = default;
  explicit SeekRequest(double t) : seconds(t) {}
};

enum class StreamState : std::uint8_t { PLAYING, DISCONTINUOUS, END_OF_STREAM };

struct DecoderData {
  std::array<
      float,
      static_cast<size_t>(audioapi::RENDER_QUANTUM_SIZE) *
          static_cast<size_t>(audioapi::MAX_CHANNEL_COUNT)>
      interleavedBuffer{};
  size_t size{};
  double timestamp{0.0};
  StreamState state{StreamState::PLAYING};
};

inline constexpr auto COMMAND_OVERFLOW_STRATEGY = OverflowStrategy::OVERWRITE_ON_FULL;
inline constexpr auto COMMAND_WAIT_STRATEGY = WaitStrategy::ATOMIC_WAIT;
inline constexpr auto COMMAND_CHANNEL_CAPACITY = 16;

using CommandSender = Sender<SeekRequest, COMMAND_OVERFLOW_STRATEGY, COMMAND_WAIT_STRATEGY>;
using CommandReceiver = Receiver<SeekRequest, COMMAND_OVERFLOW_STRATEGY, COMMAND_WAIT_STRATEGY>;

inline constexpr auto FRAME_OVERFLOW_STRATEGY = OverflowStrategy::WAIT_ON_FULL;
inline constexpr auto FRAME_WAIT_STRATEGY = WaitStrategy::ATOMIC_WAIT;
inline constexpr auto FRAME_CHANNEL_CAPACITY = 64;

using FrameSender = Sender<DecoderData, FRAME_OVERFLOW_STRATEGY, FRAME_WAIT_STRATEGY>;
using FrameReceiver = Receiver<DecoderData, FRAME_OVERFLOW_STRATEGY, FRAME_WAIT_STRATEGY>;

inline constexpr auto SLEEP_DURATION_ON_FULL = std::chrono::milliseconds(10);

namespace audioapi {

/// @brief SeekDecoderDaemon is a dedicated thread worker that manages an audio decoder instance (FFmpeg or MiniAudio).
/// It listens for seek commands from the JS thread, performs seeks on the decoder,
/// decodes audio frames, and sends decoded planar audio data back to the audio thread via a lock-free SPSC channel.
class SeekDecoderDaemon {
 public:
  SeekDecoderDaemon(
      SeekDecoderDaemonOptions options,
      std::shared_ptr<AudioFileDecoderState> sharedState,
      CommandReceiver commandReceiver,
      FrameSender frameSender,
      std::shared_ptr<FrameReceiver> frameReceiver);

  /// @brief Main loop of the daemon thread. Listens for seek commands,
  /// decodes audio frames, and sends decoded data back to the audio thread
  /// via the frameSender SPSC channel.
  void operator()();

 private:
  std::unique_ptr<decoding::IncrementalAudioDecoder> decoder_;

  /// @brief Shared state with the AudioFileSourceNode, used for communicating decoder status, metadata, and playback position.
  std::shared_ptr<AudioFileDecoderState> sharedState_;
  /// @brief Receiving end of seek commands from the JS thread
  CommandReceiver commandReceiver_;
  /// @brief Sending end for decoded audio frames back to the audio thread
  FrameSender frameSender_;
  /// @brief Pointer to the audio thread's receiver — used only during seek to drain stale frames.
  /// Safe because the daemon thread is joined before AudioFileSourceNode is destroyed.
  std::shared_ptr<FrameReceiver> frameReceiverForDrain_;

  /// @brief Drains the command queue and performs all pending seeks.
  /// @return Latest seek request if any were processed; @c std::nullopt if no seek commands were pending.
  std::optional<SeekRequest> processSeekCommands();

  /// @brief Reads one quantum of frames from the decoder into @p data, handling EOF and loop-back.
  /// @param seekRequest if present, marks the chunk as @c StreamState::DISCONTINUOUS with that target.
  /// @return true if @p data is filled and ready to send; false if the decoder is not open or not initialized.
  bool decodeNextChunk(DecoderData &data, const std::optional<SeekRequest> &seekRequest);
};

} // namespace audioapi