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

class AudioManager;

class AudioContext : public BaseAudioContext {
 public:
  explicit AudioContext();
  explicit AudioContext(float sampleRate);
  ~AudioContext() override;

  void close();
  bool resume();
  bool suspend();

 private:
#ifdef ANDROID
  std::shared_ptr<AudioPlayer> audioPlayer_;
#else
  std::shared_ptr<IOSAudioPlayer> audioPlayer_;
#endif

  std::function<void(std::shared_ptr<AudioBus>, int)> renderAudio();
};

} // namespace audioapi
