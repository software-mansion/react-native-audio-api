#pragma once

#include <memory>
#include <functional>

namespace audioapi {
class AudioBus;

class AudioRecorder {
 public:
  explicit AudioRecorder(
    float sampleRate,
    int bufferLength,
    const std::function<void(void)> &onError,
    const std::function<void(void)> &onStatusChange,
    const std::function<void(std::shared_ptr<AudioBus>, int, double)> &onAudioReady
  )
    : sampleRate_(sampleRate),
      bufferLength_(bufferLength),
      onError_(onError),
      onStatusChange_(onStatusChange),
      onAudioReady_(onAudioReady) {}

  virtual ~AudioRecorder() = default;

  virtual void start() = 0;
  virtual void stop() = 0;

 protected:
  float sampleRate_;
  int bufferLength_;

  std::function<void(void)> onError_;
  std::function<void(void)> onStatusChange_;
  std::function<void(std::shared_ptr<AudioBus>, int, double)> onAudioReady_;
};

} // namespace audioapi
