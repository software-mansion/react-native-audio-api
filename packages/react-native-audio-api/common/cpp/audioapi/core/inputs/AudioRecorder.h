#pragma once

#include <memory>
#include <functional>

namespace audioapi {
class AudioBus;
class AudioEventHandlerRegistry;

class AudioRecorder {
 public:
  explicit AudioRecorder(
    float sampleRate,
    int bufferLength,
    const std::shared_ptr<AudioEventHandlerRegistry> &audioEventHandlerRegistry
  )
    : sampleRate_(sampleRate),
      bufferLength_(bufferLength),
      audioEventHandlerRegistry_(audioEventHandlerRegistry) {}

  virtual ~AudioRecorder() = default;

  void setOnAudioReadyCallbackId(uint64_t callbackId) {
    onAudioReadyCallbackId_ = callbackId;
  }

  virtual void start() = 0;
  virtual void stop() = 0;

 protected:
  float sampleRate_;
  int bufferLength_;

  std::shared_ptr<AudioEventHandlerRegistry> audioEventHandlerRegistry_;
  uint64_t onAudioReadyCallbackId_ = 0;
};

} // namespace audioapi
