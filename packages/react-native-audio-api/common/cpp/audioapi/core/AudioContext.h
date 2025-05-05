#pragma once

#include <audioapi/core/BaseAudioContext.h>

#include <memory>
#include <functional>

namespace audioapi {
#ifdef ANDROID
class AudioPlayer;
#else
class IOSAudioPlayer;
#endif

class AudioContext : public BaseAudioContext {
 public:
  explicit AudioContext(float sampleRate, const std::shared_ptr<AudioEventHandlerRegistry> &eventHandlerRegistry);
  ~AudioContext() override;

  void close();
  bool resume();
  bool suspend();

 private:
#ifdef ANDROID
  std::shared_ptr<AudioPlayer> audioPlayer_;
#else
  std::shared_ptr<IOSAudioPlayer> audioPlayer_;
#endif

  std::function<void(std::shared_ptr<AudioBus>, int)> renderAudio();
};

} // namespace audioapi
