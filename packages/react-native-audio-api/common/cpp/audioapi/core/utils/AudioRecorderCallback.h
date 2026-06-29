#pragma once

#include <audioapi/events/EventCaller.hpp>
#include <audioapi/utils/AudioArray.hpp>
#include <audioapi/utils/AudioBuffer.hpp>
#include <audioapi/utils/CircularArray.hpp>
#include <audioapi/utils/Result.hpp>
#include <audioapi/utils/SpscChannel.hpp>
#include <audioapi/utils/TaskOffloader.hpp>
#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace audioapi {

class AudioEventHandlerRegistry;

class AudioRecorderCallback {
 public:
  AudioRecorderCallback(
      const std::shared_ptr<AudioEventHandlerRegistry> &audioEventHandlerRegistry,
      float sampleRate,
      size_t bufferLength,
      int channelCount,
      uint64_t callbackId);
  AudioRecorderCallback(const AudioRecorderCallback &) = delete;
  AudioRecorderCallback(AudioRecorderCallback &&) = delete;
  AudioRecorderCallback &operator=(const AudioRecorderCallback &) = delete;
  AudioRecorderCallback &operator=(AudioRecorderCallback &&) = delete;
  virtual ~AudioRecorderCallback();

  virtual void cleanup() = 0;

  void emitAudioData(bool flush = false);
  void invokeCallback(const std::shared_ptr<AudioBuffer> &buffer, int numFrames);

  void setOnErrorCallback(uint64_t callbackId) {
    assignOnErrorCallbackId(callbackId);
  }
  void clearOnErrorCallback() {
    assignOnErrorCallbackId(0);
  }
  void assignOnErrorCallbackId(uint64_t callbackId);
  void invokeOnErrorCallback(const std::string &message);

 protected:
  std::atomic<bool> isInitialized_{false};

  float sampleRate_;
  size_t bufferLength_;
  int channelCount_;
  size_t ringBufferSize_;
  uint64_t framesEmitted_ = 0;

  EventCaller<AudioEvent::AUDIO_READY> audioReadyEvent_;
  EventCaller<AudioEvent::RECORDER_ERROR> errorEvent_;

  // TODO: CircularAudioBuffer
  static constexpr size_t DEFAULT_RING_BUFFER_SIZE = 8192;
  std::vector<std::shared_ptr<CircularAudioArray>> circularBuffer_;
  static constexpr auto RECORDER_CALLBACK_SPSC_OVERFLOW_STRATEGY =
      channels::spsc::OverflowStrategy::OVERWRITE_ON_FULL;
  static constexpr auto RECORDER_CALLBACK_SPSC_WAIT_STRATEGY =
      channels::spsc::WaitStrategy::ATOMIC_WAIT;
  static constexpr size_t RECORDER_CALLBACK_POOL_SIZE = 32;
  // SPSC WAIT_ON_FULL rings hold at most (capacity - 1) elements.
  static constexpr auto RECORDER_CALLBACK_CHANNEL_CAPACITY = RECORDER_CALLBACK_POOL_SIZE + 1;
  // At most POOL_SIZE slots can be in flight at once, so sizing the channel one
  // larger guarantees the ring is never full when a slot is available — which is
  // why the producer can use the blocking send() without it ever actually waiting.
  static_assert(
      RECORDER_CALLBACK_POOL_SIZE <= RECORDER_CALLBACK_CHANNEL_CAPACITY - 1,
      "Channel must hold every in-flight slot so send() never blocks/overwrites");
  std::mutex destructionAudioGuard_; // eliminates race between deconstruction and audio thread
};

} // namespace audioapi
