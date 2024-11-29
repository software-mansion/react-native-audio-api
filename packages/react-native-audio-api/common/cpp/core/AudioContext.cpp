#ifdef ANDROID
#include "AudioPlayer.h"
#else
#include "IOSAudioPlayer.h"
#endif

#include "AudioContext.h"


namespace audioapi {

AudioContext::AudioContext() : BaseAudioContext() {
  audioPlayer_->start();
}

void AudioContext::close() {
  state_ = ContextState::CLOSED;
  audioPlayer_->stop();
}

} // namespace audioapi
