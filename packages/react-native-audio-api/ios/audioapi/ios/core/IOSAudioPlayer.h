#pragma once

#ifdef __OBJC__ // when compiled as Objective-C++
#import <AudioPlayer.h>
#else // when compiled as C++
typedef struct objc_object AudioPlayer;
#endif // __OBJC__

#include <functional>

namespace audioapi {

class AudioBus;
class AudioContext;
class IOSAudioManagerBridge;

class IOSAudioPlayer {
 protected:
  std::shared_ptr<AudioBus> audioBus_;
  AudioPlayer *audioPlayer_;
  std::function<void(std::shared_ptr<AudioBus>, int)> renderAudio_;
  int channelCount_;

 public:
  explicit IOSAudioPlayer(
      const std::shared_ptr<IOSAudioManagerBridge> audioManagerBridge,
      const std::function<void(std::shared_ptr<AudioBus>, int)> &renderAudio);
  IOSAudioPlayer(
      const std::shared_ptr<IOSAudioManagerBridge> audioManagerBridge,
      const std::function<void(std::shared_ptr<AudioBus>, int)> &renderAudio,
      float sampleRate);
  ~IOSAudioPlayer();

  float getSampleRate() const;

  void start();
  void stop();
  void resume();
  void suspend();
};
} // namespace audioapi
