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
  AudioContext();
  explicit AudioContext(float sampleRate);
  ~AudioContext() override;

  void close();
  void resume();
  void suspend();

 private:
#ifdef ANDROID
  std::shared_ptr<AudioPlayer> audioPlayer_;
#else
  std::shared_ptr<IOSAudioPlayer> audioPlayer_;
#endif

  std::function<void(std::shared_ptr<AudioBus>, int)> renderAudio();
};

} // namespace audioapi
