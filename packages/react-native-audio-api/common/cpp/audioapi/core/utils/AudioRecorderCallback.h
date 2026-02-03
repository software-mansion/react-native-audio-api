#pragma once

#include <audioapi/utils/Result.hpp>
#include <audioapi/utils/SpscChannel.hpp>
#include <audioapi/utils/TaskOffloader.hpp>
#include <atomic>
#include <memory>
#include <string>
#include <vector>

namespace audioapi {

class AudioBus;
class AudioArray;
class CircularAudioArray;
class AudioEventHandlerRegistry;

class AudioRecorderCallback {
 public:
  AudioRecorderCallback(
      const std::shared_ptr<AudioEventHandlerRegistry> &audioEventHandlerRegistry,
      float sampleRate,
      size_t bufferLength,
      int channelCount,
      uint64_t callbackId);
  virtual ~AudioRecorderCallback();

  virtual void cleanup() = 0;

  void emitAudioData(bool flush = false);
  void invokeCallback(const std::shared_ptr<AudioBus> &bus, int numFrames);

  void setOnErrorCallback(uint64_t callbackId);
  void clearOnErrorCallback();
  void invokeOnErrorCallback(const std::string &message);

 protected:
  std::atomic<bool> isInitialized_{false};

  float sampleRate_;
  size_t bufferLength_;
  int channelCount_;
  uint64_t callbackId_;
  size_t ringBufferSize_;

  std::atomic<uint64_t> errorCallbackId_{0};

  std::shared_ptr<AudioEventHandlerRegistry> audioEventHandlerRegistry_;

  // TODO: CircularAudioBus
  std::vector<std::shared_ptr<CircularAudioArray>> circularBus_;
  static constexpr auto RECORDER_CALLBACK_SPSC_OVERFLOW_STRATEGY =
      channels::spsc::OverflowStrategy::OVERWRITE_ON_FULL;
  static constexpr auto RECORDER_CALLBACK_SPSC_WAIT_STRATEGY =
      channels::spsc::WaitStrategy::ATOMIC_WAIT;
  static constexpr auto RECORDER_CALLBACK_CHANNEL_CAPACITY = 64;
};

} // namespace audioapi
