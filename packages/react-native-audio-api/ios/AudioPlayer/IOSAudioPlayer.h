#pragma once

#ifdef __OBJC__ // when compiled as Objective-C++
#import <AudioPlayer.h>
#else // when compiled as C++
typedef struct objc_object AudioPlayer;
#endif // __OBJC__

namespace audioapi {
class IOSAudioPlayer {
 protected:
  AudioPlayer *audioPlayer_;

 public:
  explicit IOSAudioPlayer();
  ~IOSAudioPlayer();
  
  int getSampleRate() const;
  int getFramesPerBurst() const;
  void start();
  void stop();
  void renderAudio(float *audioData, int32_t numFrames);
  
};
} // namespace audioapi
