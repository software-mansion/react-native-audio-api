#ifdef ANDROID
#include "AudioPlayer.h"
#else
#include "IOSAudioPlayer.h"
#endif

#include "AudioContext.h"


namespace audioapi {

AudioContext::AudioContext() : BaseAudioContext() {}

void AudioContext::close() {
  state_ = State::CLOSED;

  if (audioPlayer_) {
    audioPlayer_->stop();
  }

  audioPlayer_.reset();
  destination_.reset();
}
} // namespace audioapi
