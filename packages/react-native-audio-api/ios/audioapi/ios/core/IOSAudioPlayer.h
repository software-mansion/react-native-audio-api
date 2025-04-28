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

class IOSAudioPlayer {
  public:
    IOSAudioPlayer(
        const std::function<void(std::shared_ptr<AudioBus>, int)> &renderAudio,
        float sampleRate);
    ~IOSAudioPlayer();

    void start();
    void resume();
    void stop();
    void pause();
  
  protected:
    std::shared_ptr<AudioBus> audioBus_;
    AudioPlayer *audioPlayer_;
    std::function<void(std::shared_ptr<AudioBus>, int)> renderAudio_;
    int channelCount_;
    std::atomic<bool> isRunning_;

};
} // namespace audioapi
