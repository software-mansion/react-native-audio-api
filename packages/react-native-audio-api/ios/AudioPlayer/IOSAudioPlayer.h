#pragma once

#ifdef __OBJC__ // when compiled as Objective-C++
#import <AudioPlayer.h>
#else // when compiled as C++
typedef struct objc_object AudioPlayer;
#endif // __OBJC__

#include <functional>

namespace audioapi {

class AudioContext;

class IOSAudioPlayer {
 protected:
  AudioPlayer *audioPlayer_;
  std::function<void(float*, int)> renderAudio_;

 public:
  explicit IOSAudioPlayer(const std::function<void(float*, int)> &renderAudio);
  ~IOSAudioPlayer();
  
  int getSampleRate() const;
  void start();
  void stop();
  void renderAudio(float *audioData, int32_t numFrames);
};
} // namespace audioapi
