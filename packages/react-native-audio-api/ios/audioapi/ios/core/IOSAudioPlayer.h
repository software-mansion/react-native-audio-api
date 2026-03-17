#pragma once

#ifdef __OBJC__ // when compiled as Objective-C
#import <NativeAudioPlayer.h>
#else  // when compiled as C++
typedef struct objc_object NativeAudioPlayer;
#endif // __OBJC__

#include <functional>

namespace audioapi {

class AudioBuffer;
class AudioContext;

class IOSAudioPlayer {
 public:
  IOSAudioPlayer(
      const std::function<void(std::shared_ptr<AudioBuffer>, int)> &renderAudio,
      float sampleRate,
      int channelCount);
  ~IOSAudioPlayer();

  bool start();
  void stop();
  bool resume();
  void suspend();
  void cleanup();

  bool isRunning() const;

 protected:
  std::shared_ptr<AudioBuffer> audioBuffer_;
  NativeAudioPlayer *audioPlayer_;
  std::function<void(std::shared_ptr<AudioBuffer>, int)> renderAudio_;
  int channelCount_;
  std::atomic<bool> isRunning_;
};

} // namespace audioapi
