#pragma once

#include <audioapi/core/utils/Constants.h>
#include <audioapi/dsp/r8brain/Resampler.hpp>
#include <audioapi/events/EventCaller.hpp>
#include <audioapi/utils/AudioArray.hpp>
#include <audioapi/utils/AudioBuffer.hpp>
#include <audioapi/utils/AudioBufferPool.hpp>
#include <audioapi/utils/CircularArray.hpp>
#include <audioapi/utils/Macros.h>
#include <audioapi/utils/Result.hpp>
#include <audioapi/utils/SpscChannel.hpp>
#include <audioapi/utils/TaskOffloader.hpp>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace audioapi {

class IAudioEventHandlerRegistry;

/// A filled pool buffer plus frame count — the only thing that crosses to the worker thread.
/// The default-constructed value (no buffer) is the offloader's shutdown message, which is
/// what operator== exists for.
struct PendingCallbackFrames {
  AudioBufferLease buffer;
  int numFrames = 0;

  bool operator==(const PendingCallbackFrames &) const = default;
};

/// Delivers recorded audio to a JS `onAudioReady` callback, resampling and remixing from
/// the input stream's format to the one JS asked for. The audio thread only copies into a
/// preallocated slot; conversion and dispatch happen on a worker thread.
class AudioRecorderCallback {
 public:
  AudioRecorderCallback(
      const std::shared_ptr<IAudioEventHandlerRegistry> &audioEventHandlerRegistry,
      float sampleRate,
      size_t bufferLength,
      int channelCount,
      uint64_t callbackId);
  DELETE_COPY_AND_MOVE(AudioRecorderCallback);
  ~AudioRecorderCallback();

  /// JS thread. Builds the converter and the audio-thread → worker pool.
  Result<NoneType, std::string>
  prepare(float streamSampleRate, int streamChannelCount, size_t maxInputBufferLength);
  void cleanup();

  /// Audio thread. @p channels holds one pointer per stream channel, each to numFrames float32
  /// samples, valid only for the duration of the call.
  void receiveAudioData(const float *const *channels, int numFrames);

  void emitAudioData(bool flush = false);
  void invokeCallback(const std::shared_ptr<AudioBuffer> &buffer, int numFrames);

  void setOnErrorCallback(uint64_t callbackId) {
    assignOnErrorCallbackId(callbackId);
  }
  void clearOnErrorCallback() {
    assignOnErrorCallbackId(0);
  }
  void invokeOnErrorCallback(const std::string &message);

 private:
  void assignOnErrorCallbackId(uint64_t callbackId);

  static constexpr auto RECORDER_CALLBACK_SPSC_OVERFLOW_STRATEGY =
      channels::spsc::OverflowStrategy::OVERWRITE_ON_FULL;
  static constexpr auto RECORDER_CALLBACK_SPSC_WAIT_STRATEGY =
      channels::spsc::WaitStrategy::ATOMIC_WAIT;
  static constexpr size_t RECORDER_CALLBACK_POOL_SIZE = 32;
  // SPSC rings hold at most (capacity - 1) elements.
  static constexpr auto RECORDER_CALLBACK_CHANNEL_CAPACITY = RECORDER_CALLBACK_POOL_SIZE + 1;
  // At most POOL_SIZE slots can be in flight at once, so sizing the channel one
  // larger guarantees the ring is never full when a slot is available — which is
  // why the producer can use the blocking send() without it ever actually waiting.
  static_assert(
      RECORDER_CALLBACK_POOL_SIZE <= RECORDER_CALLBACK_CHANNEL_CAPACITY - 1,
      "Channel must hold every in-flight slot so send() never blocks/overwrites");

  using Offloader = task_offloader::TaskOffloader<
      PendingCallbackFrames,
      RECORDER_CALLBACK_SPSC_OVERFLOW_STRATEGY,
      RECORDER_CALLBACK_SPSC_WAIT_STRATEGY>;

  /// r8brain resamplers are built for a fixed maximum input block, so longer callbacks
  /// are fed through in chunks of this size.
  static constexpr int RESAMPLER_MAX_INPUT_FRAMES = 2048;

  void runCallbackTask(PendingCallbackFrames pending);
  /// Remixes and/or resamples @p slotFrames chunk by chunk into the circular buffer.
  void convertAndPushChunks(const AudioBuffer &slotFrames, int numFrames);
  void pushChannels(const AudioBuffer &planarFrames, int numFrames);
  void pushChannels(const float *const *planarFrames, int numFrames);
  void releaseProcessingResources();

  std::atomic<bool> isInitialized_{false};

  float sampleRate_;
  size_t bufferLength_;
  int channelCount_;
  size_t ringBufferSize_;
  uint64_t framesEmitted_ = 0;

  float streamSampleRate_{0.0F};
  int streamChannelCount_{0};
  size_t maxInputBufferLength_{0};

  EventCaller<AudioEvent::AUDIO_READY> audioReadyEvent_;
  EventCaller<AudioEvent::RECORDER_ERROR> errorEvent_;

  // TODO: CircularAudioBuffer
  static constexpr size_t DEFAULT_RING_BUFFER_SIZE = 8192;
  std::vector<std::shared_ptr<CircularAudioArray>> circularBuffer_;

  /// Conversion chain, built only for the parts that are actually needed: r8brain
  /// resamples but does not remix, so a channel-count change goes through
  /// AudioBuffer::copy first. Null when the stream already matches the callback format.
  std::unique_ptr<r8b::MultiChannelResampler> resampler_;
  std::unique_ptr<AudioBuffer> remixedChunk_; // one chunk, callback channel count
  std::unique_ptr<AudioBuffer> resamplerOutput_;

  /// Planar buffers of maxInputBufferLength_ x streamChannelCount_ that carry audio-thread
  /// callbacks to the worker.
  AudioBufferPool<RECORDER_CALLBACK_POOL_SIZE> inputBufferPool_;

  // delay initialization of offloader until prepare is called
  std::unique_ptr<Offloader> offloader_;

  std::mutex destructionAudioGuard_; // eliminates race between deconstruction and audio thread
};

} // namespace audioapi
