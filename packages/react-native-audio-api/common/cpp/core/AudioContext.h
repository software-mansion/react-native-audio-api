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

class AudioContext : public BaseAudioContext {
 public:
  AudioContext();
  explicit AudioContext(float sampleRate);
  ~AudioContext() override;

  void close();

 private:
#ifdef ANDROID
  std::shared_ptr<AudioPlayer> audioPlayer_;
#else
  std::shared_ptr<IOSAudioPlayer> audioPlayer_;
#endif

  std::function<void(AudioBus *, int)> renderAudio();
};

} // namespace audioapi
