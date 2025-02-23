#pragma once

#include <functional>

namespace audioapi {

class NowPlayingInfo;
class AudioSessionOptions;

class IOSAudioControls {
 public:
  IOSAudioControls();
  ~IOSAudioControls();

  void init(std::shared_ptr<AudioSessionOptions> options);
  void updateOptions(std::shared_ptr<AudioSessionOptions> options);
  void disable();

  void showNowPlayingInfo(std::shared_ptr<NowPlayingInfo> nowPlayingInfo);
  void updateNowPlayingInfo(std::shared_ptr<NowPlayingInfo> nowPlayingInfo);
  void hideNowPlayingInfo();

  void addEventListener();
};

} // namespace audioapi
