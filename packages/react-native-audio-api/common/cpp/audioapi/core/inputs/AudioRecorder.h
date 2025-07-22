#pragma once


#include <memory>
#include <atomic>

namespace audioapi {
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
    const std::shared_ptr<AudioEventHandlerRegistry> &audioEventHandlerRegistry
  );

  virtual ~AudioRecorder() = default;

  void setOnAudioReadyCallbackId(uint64_t callbackId);
  void invokeOnAudioReadyCallback(const std::shared_ptr<AudioBus> &bus, int numFrames, double when);
  void sendRemainingData();

  void connect(const std::shared_ptr<RecorderAdapterNode> &node);

  virtual void start() = 0;
  virtual void stop() = 0;

 protected:
  float sampleRate_;
  int bufferLength_;
  size_t ringBufferSize_;

  std::atomic<bool> isRunning_;
  std::shared_ptr<CircularAudioArray> circularBuffer_;
  std::shared_ptr<RecorderAdapterNode> adapterNode_;

  std::shared_ptr<AudioEventHandlerRegistry> audioEventHandlerRegistry_;
  uint64_t onAudioReadyCallbackId_ = 0;

  void writeToBuffers(const float *data, int numFrames);
};

} // namespace audioapi
