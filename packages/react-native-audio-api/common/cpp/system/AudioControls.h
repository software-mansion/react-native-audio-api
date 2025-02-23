#pragma once

#include <memory>
#include <functional>

namespace audioapi {

class NowPlayingInfo;
class AudioSessionOptions;

#ifdef ANDROID
class AndroidAudioControls;
#else
class IOSAudioControls;
#endif

class AudioControls {
 public:
  AudioControls();
  ~AudioControls();

  void init(std::shared_ptr<AudioSessionOptions> options);
  void updateOptions(std::shared_ptr<AudioSessionOptions> options);
  void disable();

  void showNowPlayingInfo(std::shared_ptr<NowPlayingInfo> nowPlayingInfo);
  void updateNowPlayingInfo(std::shared_ptr<NowPlayingInfo> nowPlayingInfo);
  void hideNowPlayingInfo();

  void addEventListener();

 private:
#ifdef ANDROID
  std::shared_ptr<AndroidAudioControls> audioControls_;
#else
  std::shared_ptr<IOSAudioControls> audioControls_;
#endif
};

} // namespace audioapi
