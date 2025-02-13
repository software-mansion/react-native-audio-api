#pragma once

#ifdef __OBJC__ // when compiled as Objective-C++
#import <AudioPlayer.h>
#else // when compiled as C++
typedef struct objc_object AudioPlayer;
#endif // __OBJC__

#include <functional>

namespace audioapi {

class AudioContext;
class AudioBus;

class IOSAudioPlayer {
 protected:
  AudioBus *audioBus_;
  AudioPlayer *audioPlayer_;
  std::function<void(AudioBus *, int)> renderAudio_;

 public:
  explicit IOSAudioPlayer(
      const std::function<void(AudioBus *, int)> &renderAudio);
  IOSAudioPlayer(
      const std::function<void(AudioBus *, int)> &renderAudio,
      float sampleRate);
  ~IOSAudioPlayer();

  float getSampleRate() const;

  void start();
  void stop();
};
} // namespace audioapi
