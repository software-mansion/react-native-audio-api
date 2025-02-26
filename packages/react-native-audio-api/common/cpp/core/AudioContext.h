#pragma once

#include <memory>
#include <functional>

#include "BaseAudioContext.h"

namespace audioapi {
#ifdef ANDROID
class AudioPlayer;
#else
class IOSAudioPlayer;
#endif

class AudioManager;

class AudioContext : public BaseAudioContext {
 public:
  explicit AudioContext();
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
