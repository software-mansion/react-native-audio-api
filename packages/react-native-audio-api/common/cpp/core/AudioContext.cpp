#include "AudioContext.h"

namespace audioapi {

AudioContext::AudioContext() : BaseAudioContext() {}

void AudioContext::close() {
  state_ = ContextState::CLOSED;

  if (audioPlayer_) {
    audioPlayer_->stop();
  }

  audioPlayer_.reset();
  destination_.reset();
}
} // namespace audioapi
