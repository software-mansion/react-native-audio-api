#pragma once

#include <audioapi/libs/decoding/IncrementalAudioDecoder.h>
#if !RN_AUDIO_API_FFMPEG_DISABLED
#include <audioapi/libs/ffmpeg/FFmpegDecoding.h>
#endif // RN_AUDIO_API_FFMPEG_DISABLED
#include <audioapi/libs/miniaudio/MiniAudioDecoding.h>
#include <audioapi/utils/SpscChannel.hpp>
#include <memory>
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

// TODO: check if all those fields are necessary or even used
struct AudioFileDecoderState {
  // Lifecycle and Sync
  std::atomic<bool> isDaemonRunning{true};
  std::atomic<bool> isReady{false}; // True once the decoder opens the file/URL
  std::atomic<int> pendingOffloadedSeeks{0};
  std::atomic<bool> isEof{false};

  // Metadata
  std::atomic<int> channelCount{0};
  std::atomic<float> sampleRate{0.0f};
  std::atomic<double> duration{0.0};

  std::atomic<bool> onPositionChangedFlush{false};

  // Playback state
  std::atomic<double> currentTime{0.0};
  std::atomic<bool> loop{false};
};

struct SeekRequest {
  double seconds = 0;
  SeekRequest() = default;
  explicit SeekRequest(double t) : seconds(t) {}
};

struct DecoderData {
  std::vector<float> interleavedBuffer;
  size_t size{};
};

namespace audioapi {

using CommandReceiver =
    Receiver<SeekRequest, OverflowStrategy::OVERWRITE_ON_FULL, WaitStrategy::ATOMIC_WAIT>;
using FrameSender = Sender<DecoderData, OverflowStrategy::WAIT_ON_FULL, WaitStrategy::ATOMIC_WAIT>;

/// @brief SeekDecoderDaemon is a dedicated thread worker that manages an audio decoder instance (FFmpeg or MiniAudio).
/// It listens for seek commands from the JS thread, performs seeks on the decoder,
/// decodes audio frames, and sends decoded planar audio data back to the audio thread via a lock-free SPSC channel.
class SeekDecoderDaemon {
 public:
  SeekDecoderDaemon(
      SeekDecoderDaemonOptions options,
      std::shared_ptr<AudioFileDecoderState> sharedState,
      CommandReceiver commandReceiver,
      FrameSender frameSender);

  /// @brief Main loop of the daemon thread. Listens for seek commands,
  /// decodes audio frames, and sends decoded data back to the audio thread
  /// via the frameSender SPSC channel.
  void operator()();

 private:
  std::shared_ptr<AudioFileDecoderState> sharedState_;
  std::unique_ptr<decoding::IncrementalAudioDecoder> decoder_;
  CommandReceiver commandReceiver_;
  FrameSender frameSender_;
};

} // namespace audioapi