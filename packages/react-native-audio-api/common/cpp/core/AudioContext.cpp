#ifdef ANDROID
#include "AudioPlayer.h"
#else
#include "IOSAudioPlayer.h"
#endif

#include "AudioContext.h"
#include "AudioDecoder.h"
#include "AudioDestinationNode.h"
#include "AudioNodeManager.h"

namespace audioapi {
AudioContext::AudioContext() : BaseAudioContext() {
#ifdef ANDROID
  audioPlayer_ = std::make_shared<AudioPlayer>(this->renderAudio());
#else
  audioPlayer_ = std::make_shared<IOSAudioPlayer>(this->renderAudio());
#endif
  sampleRate_ = audioPlayer_->getSampleRate();
  audioDecoder_ = std::make_shared<AudioDecoder>(sampleRate_);

  audioPlayer_->start();
}

AudioContext::AudioContext(float sampleRate) : BaseAudioContext() {
#ifdef ANDROID
  audioPlayer_ = std::make_shared<AudioPlayer>(this->renderAudio(), sampleRate);
#else
  audioPlayer_ =
      std::make_shared<IOSAudioPlayer>(this->renderAudio(), sampleRate);
#endif
  sampleRate_ = audioPlayer_->getSampleRate();
  audioDecoder_ = std::make_shared<AudioDecoder>(sampleRate_);

  audioPlayer_->start();
}

AudioContext::~AudioContext() {
  if (!isClosed()) {
    close();
  }

  nodeManager_->cleanup();
}

void AudioContext::close() {
  state_ = ContextState::CLOSED;
  audioPlayer_->stop();
}

void AudioContext::resume() {
  state_ = ContextState::RUNNING;
  audioPlayer_->resume();
}

void AudioContext::suspend() {
  state_ = ContextState::SUSPENDED;
  audioPlayer_->suspend();
}

std::function<void(std::shared_ptr<AudioBus>, int)>
AudioContext::renderAudio() {
  if (!isRunning() || !destination_) {
    return [](const std::shared_ptr<AudioBus> &, int) {};
  }

  return [this](const std::shared_ptr<AudioBus> &data, int frames) {
    destination_->renderAudio(data, frames);
  };
}

} // namespace audioapi
