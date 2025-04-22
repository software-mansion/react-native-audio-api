#pragma once

#include <memory>
#include <functional>

namespace audioapi {
class AudioBus;

#ifdef ANDROID

#else
class IOSAudioRecorder;
#endif

class AudioRecorder {
 public:
  explicit AudioRecorder(
    float sampleRate,
    int numberOfChannels,
    int bufferLength,
    bool enableVoiceProcessing,
    std::function<void(void)> onError,
    std::function<void(void)> onStatusChange,
    std::function<void(std::shared_ptr<AudioBus>, int, double)> onAudioReady
  );

  ~AudioRecorder();

  void start();
  void stop();

 private:
#ifdef ANDROID

#else
  std::shared_ptr<IOSAudioRecorder> audioRecorder_;
#endif

  std::function<void(void)> onError_;
  std::function<void(void)> onStatusChange_;
  std::function<void(std::shared_ptr<AudioBus>, int, double)> onAudioReady_;

  std::function<void(std::shared_ptr<AudioBus>, int, double)> getOnAudioReady();
};

} // namespace audioapi
