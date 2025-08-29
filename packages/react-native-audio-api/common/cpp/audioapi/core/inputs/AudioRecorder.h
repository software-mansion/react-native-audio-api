#pragma once

#include <jsi/jsi.h>
#include <audioapi/core/utils/worklets/UiWorkletsRunner.h>

#include <memory>
#include <atomic>
#include <mutex>

namespace audioapi {
using namespace facebook;

class RecorderAdapterNode;
class AudioBus;
class CircularAudioArray;
class CircularOverflowableAudioArray;
class AudioEventHandlerRegistry;

class AudioRecorder {
 public:
  explicit AudioRecorder(
    float sampleRate,
    int bufferLength,
    const std::shared_ptr<AudioEventHandlerRegistry> &audioEventHandlerRegistry,
    const std::shared_ptr<UiWorkletsRunner> &workletRunner
  );

  virtual ~AudioRecorder() = default;

  void setOnAudioReadyCallbackId(uint64_t callbackId);
  void invokeOnAudioReadyCallback(const std::shared_ptr<AudioBus> &bus, int numFrames, double when);
  void sendRemainingData();

  /// @brief
  /// # Connects the recorder to the adapter node.
  ///
  /// The adapter node will be used to read audio data from the recorder.
  /// @note Few frames of audio might not yet be written to the buffer when connecting.
  void connect(const std::shared_ptr<RecorderAdapterNode> &node);

  /// @brief
  /// # Disconnects the recorder from the adapter node.
  ///
  /// The adapter node will no longer be used to read audio data from the recorder.
  /// @note Last few frames of audio might be written to the buffer after disconnecting.
  void disconnect();


  #if RN_AUDIO_API_ENABLE_WORKLETS
  void invokeWorkletOnAudioReadyCallback(const std::shared_ptr<AudioBus> &bus,
                                        int numFrames, double when);
  void setWorkletCallback(std::shared_ptr<worklets::ShareableWorklet> &callback);
  #endif

  virtual void start() = 0;
  virtual void stop() = 0;

 protected:
  float sampleRate_;
  int bufferLength_;
  size_t ringBufferSize_;

  std::atomic<bool> isRunning_;
  std::shared_ptr<CircularAudioArray> circularBuffer_;

  mutable std::mutex adapterNodeLock_;
  std::shared_ptr<RecorderAdapterNode> adapterNode_ = nullptr;

  std::shared_ptr<AudioEventHandlerRegistry> audioEventHandlerRegistry_;
  uint64_t onAudioReadyCallbackId_ = 0;
  std::shared_ptr<UiWorkletsRunner> workletRunner_;


  #if RN_AUDIO_API_ENABLE_WORKLETS
  std::shared_ptr<worklets::ShareableWorklet> shareableWorklet_;
  mutable std::mutex workletCallbackMutex_;
  #endif

  void writeToBuffers(const float *data, int numFrames);
};

} // namespace audioapi
