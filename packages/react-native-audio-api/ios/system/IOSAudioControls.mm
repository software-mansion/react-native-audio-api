#include "NowPlayingInfo.h"
#include "IOSAudioControls.h"
#include "AudioSessionOptions.h"

namespace audioapi {

IOSAudioControls::IOSAudioControls() {
  printf("IOSAudioControls::IOSAudioControls()\n");
}

IOSAudioControls::~IOSAudioControls() {
  printf("IOSAudioControls::~IOSAudioControls()\n");
}

void IOSAudioControls::init(std::shared_ptr<AudioSessionOptions> options) {
}

void IOSAudioControls::updateOptions(std::shared_ptr<AudioSessionOptions> options) {
  printf("IOSAudioControls::updateOptions()\n");
}

void IOSAudioControls::disable() {
  printf("IOSAudioControls::disable()\n");
}

void IOSAudioControls::showNowPlayingInfo(std::shared_ptr<NowPlayingInfo> info) {
  printf("IOSAudioControls::showNowPlayingInfo()\n");
}

void IOSAudioControls::updateNowPlayingInfo(std::shared_ptr<NowPlayingInfo> info) {
  printf("IOSAudioControls::updateNowPlayingInfo()\n");
}

void IOSAudioControls::hideNowPlayingInfo() {
  printf("IOSAudioControls::hideNowPlayingInfo()\n");
}

void IOSAudioControls::addEventListener() {
  printf("IOSAudioControls::addEventListener()\n");
}

} // namespace audioapi
