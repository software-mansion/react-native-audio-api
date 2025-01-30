#pragma once

#include <memory>

#include "BaseAudioContext.h"

namespace audioapi {

#ifdef ANDROID
    class AudioPlayer;
#else
    class IOSAudioPlayer;
#endif

class AudioContext : public BaseAudioContext {
 public:
  AudioContext();
  ~AudioContext() override;

  void close();

  std::function<void(AudioBus *, int)> renderAudio();

private:

#ifdef ANDROID
    std::shared_ptr<AudioPlayer> audioPlayer_;
#else
    std::shared_ptr<IOSAudioPlayer> audioPlayer_;
#endif
};

} // namespace audioapi
