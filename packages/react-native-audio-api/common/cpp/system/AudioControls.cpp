
#include "AudioControls.h"
#include "NowPlayingInfo.h"
#include "AudioSessionOptions.h"

#ifdef ANDROID
#include "AndroidAudioControls.h"
#else
#include "IOSAudioControls.h"
#endif

namespace audioapi {

AudioControls::AudioControls() {
#ifdef ANDROID
  audioControls_ = std::make_shared<AndroidAudioControls>();
#else
  audioControls_ = std::make_shared<IOSAudioControls>();
#endif
}

AudioControls::~AudioControls() {
  // TODO: cleanup everything if disable was not called
}

void AudioControls::init(std::shared_ptr<AudioSessionOptions> options) {
  audioControls_->init(options);
}

void AudioControls::updateOptions(std::shared_ptr<AudioSessionOptions> options) {
  audioControls_->updateOptions(options);
}

void AudioControls::disable() {
  audioControls_->disable();
}

void AudioControls::showNowPlayingInfo(std::shared_ptr<NowPlayingInfo> nowPlayingInfo) {
  audioControls_->showNowPlayingInfo(nowPlayingInfo);
}

void AudioControls::updateNowPlayingInfo(std::shared_ptr<NowPlayingInfo> nowPlayingInfo) {
  audioControls_->updateNowPlayingInfo(nowPlayingInfo);
}

void AudioControls::hideNowPlayingInfo() {
  audioControls_->hideNowPlayingInfo();
}

void AudioControls::addEventListener() {
  audioControls_->addEventListener();
}

} // namespace audioapi
