#pragma once

#include <audioapi/core/AudioNode.h>
#include <audioapi/core/sources/AudioScheduledSourceNode.h>
#include <audioapi/core/utils/decoding/SeekDecoderDaemon.h>
#include <audioapi/libs/decoding/IncrementalAudioDecoder.h>
#include <cstddef>
#include <thread>
#if !RN_AUDIO_API_FFMPEG_DISABLED
#include <audioapi/libs/ffmpeg/FFmpegDecoding.h>
#endif // RN_AUDIO_API_FFMPEG_DISABLED
#include <audioapi/libs/miniaudio/MiniAudioDecoding.h>

#include <atomic>
#include <memory>

using namespace audioapi::channels;

namespace audioapi {

struct AudioFileSourceOptions;

inline constexpr auto FRAME_SPSC_OVERFLOW_STRATEGY =
    audioapi::channels::spsc::OverflowStrategy::WAIT_ON_FULL;
inline constexpr auto FRAME_SPSC_WAIT_STRATEGY =
    audioapi::channels::spsc::WaitStrategy::ATOMIC_WAIT;
inline constexpr auto FRAME_SPSC_CHANNEL_CAPACITY = 32;

inline constexpr auto COMMAND_SPSC_OVERFLOW_STRATEGY =
    audioapi::channels::spsc::OverflowStrategy::OVERWRITE_ON_FULL;
inline constexpr auto COMMAND_SPSC_WAIT_STRATEGY =
    audioapi::channels::spsc::WaitStrategy::ATOMIC_WAIT;
inline constexpr auto COMMAND_SPSC_CHANNEL_CAPACITY = 16;

inline constexpr auto ON_POSITION_CHANGED_INTERVAL = 0.25f;

class AudioFileSourceNode : public AudioScheduledSourceNode {
 public:
  explicit AudioFileSourceNode(
      const std::shared_ptr<BaseAudioContext> &context,
      const AudioFileSourceOptions &options);
  ~AudioFileSourceNode() override = default;

  void disable() override;

  void start(double when) override;

  void setVolume(float v) {
    volume_ = v;
  }

  void pause();

  /// @note Audio Thread only
  void setOnPositionChangedCallbackId(uint64_t callbackId);
  void unregisterOnPositionChangedCallback(uint64_t callbackId);

  void setLoop(bool v) {
    loop_ = v;
  }

  double getDuration() const {
    return duration_;
  }

  double getCurrentTime() const {
    return currentTime_.load(std::memory_order_acquire);
  }

  void seekToTime(double seconds);

 protected:
  std::shared_ptr<DSPAudioBuffer> processNode(
      const std::shared_ptr<DSPAudioBuffer> &processingBuffer,
      int framesToProcess) override;

 private:
  std::shared_ptr<AudioFileDecoderState> decoderState_;
  float volume_;
  bool filePaused_{false};
  bool loop_{false};
  double duration_{0};
  double sampleRate_{0};
  std::atomic<double> currentTime_{0};

  size_t readInterleavedFrames(
      const std::shared_ptr<DSPAudioBuffer> &destBuffer,
      size_t framesToRead);

  bool seekDecoderToTime(double seconds);

  void sendOnPositionChangedEvent(int framesPlayed);

  void applyPlaybackStateAfterSuccessfulSeek(double seconds);

  // Daemon thread for decoding and seeking
  std::unique_ptr<SeekDecoderDaemon> seekDecoderDaemon_;
  std::thread seekDecoderThread_;

  uint64_t onPositionChangedCallbackId_ = 0;
  int onPositionChangedInterval_;
  int onPositionChangedTime_ = 0;
  std::atomic<bool> onPositionChangedFlush_{true};

  /// SPSC for JS -> Daemon thread communication (seek event)
  channels::spsc::Sender<SeekRequest, COMMAND_SPSC_OVERFLOW_STRATEGY, COMMAND_SPSC_WAIT_STRATEGY>
      commandSender_;

  /// SPSC for Daemon thread -> Audio thread communication (decoded frames)
  channels::spsc::Receiver<DecoderData, FRAME_SPSC_OVERFLOW_STRATEGY, FRAME_SPSC_WAIT_STRATEGY>
      frameReceiver_;
};

} // namespace audioapi
