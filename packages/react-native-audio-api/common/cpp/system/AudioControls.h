#pragma once

#include <memory>
#include <functional>

namespace audioapi {

class NowPlayingInfo;
class AudioSessionOptions;

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
  std::shared_ptr<AudioSessionOptions> options_;
  std::shared_ptr<NowPlayingInfo> nowPlayingInfo_;
};

} // namespace audioapi
