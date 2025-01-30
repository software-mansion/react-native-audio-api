#ifdef ANDROID
#include "AudioPlayer.h"
#else
#include "IOSAudioPlayer.h"
#endif

#include "AudioContext.h"
#include "AudioDestinationNode.h"

namespace audioapi {

AudioContext::AudioContext() : BaseAudioContext() {
#ifdef ANDROID
  audioPlayer_ = std::make_shared<AudioPlayer>(this->renderAudio());
#else
  audioPlayer_ = std::make_shared<IOSAudioPlayer>(this->renderAudio());
#endif
  sampleRate_ = audioPlayer_->getSampleRate();

  audioPlayer_->start();
}

AudioContext::~AudioContext() {
  if (isRunning()) {
    return;
  }

  close();
}

void AudioContext::close() {
  state_ = ContextState::CLOSED;
  audioPlayer_->stop();
}

std::function<void(AudioBus *, int)> AudioContext::renderAudio() {
  if (!isRunning()) {
    return [](AudioBus *, int) {};
  }

  return [this](AudioBus *data, int frames) {
    destination_->renderAudio(data, frames);
  };
}

} // namespace audioapi
