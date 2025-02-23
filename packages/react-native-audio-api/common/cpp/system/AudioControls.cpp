
#include "AudioControls.h"
#include "NowPlayingInfo.h"
#include "AudioSessionOptions.h"

namespace audioapi {

AudioControls::AudioControls() {
  printf("AudioControls::AudioControls\n");
}

AudioControls::~AudioControls() {
  printf("AudioControls::~AudioControls\n");
}

void AudioControls::init(std::shared_ptr<AudioSessionOptions> options) {
  printf("AudioControls::init\n");
  options_ = options;
}

void AudioControls::updateOptions(std::shared_ptr<AudioSessionOptions> options) {
  printf("AudioControls::updateOptions\n");
  options_ = options;
}

void AudioControls::disable() {
  printf("AudioControls::disable\n");
}

void AudioControls::showNowPlayingInfo(std::shared_ptr<NowPlayingInfo> nowPlayingInfo) {
  printf("AudioControls::showNowPlayingInfo\n");
  nowPlayingInfo_ = nowPlayingInfo;
}

void AudioControls::updateNowPlayingInfo(std::shared_ptr<NowPlayingInfo> nowPlayingInfo) {
  printf("AudioControls::updateNowPlayingInfo\n");
  nowPlayingInfo_ = nowPlayingInfo;
}

void AudioControls::hideNowPlayingInfo() {
  printf("AudioControls::hideNowPlayingInfo\n");
}

void AudioControls::addEventListener() {
  printf("AudioControls::addEventListener\n");
}

} // namespace audioapi
