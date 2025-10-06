#ifdef ANDROID
#include <audioapi/android/core/AudioPlayer.h>
#else
#include <audioapi/ios/core/IOSAudioPlayer.h>
#endif

#include <audioapi/core/AudioContext.h>
#include <audioapi/core/destinations/AudioDestinationNode.h>
#include <audioapi/core/utils/AudioNodeManager.h>

namespace audioapi {
AudioContext::AudioContext(
    float sampleRate,
    bool initSuspended,
    const std::shared_ptr<IAudioEventHandlerRegistry>
        &audioEventHandlerRegistry,
    const RuntimeRegistry &runtimeRegistry)
    : BaseAudioContext(audioEventHandlerRegistry, runtimeRegistry) {
#ifdef ANDROID
  audioPlayer_ = std::make_shared<AudioPlayer>(
      this->renderAudio(), sampleRate, destination_->getChannelCount());
#else
  audioPlayer_ = std::make_shared<IOSAudioPlayer>(
      this->renderAudio(), sampleRate, destination_->getChannelCount());
#endif

  sampleRate_ = sampleRate;

  if (initSuspended) {
    playerHasBeenStarted_ = false;
    state_ = ContextState::SUSPENDED;

    return;
  }

  playerHasBeenStarted_ = true;
  audioPlayer_->start();
  state_ = ContextState::RUNNING;
}

AudioContext::~AudioContext() {
  if (!isClosed()) {
    close();
  }
}

void AudioContext::close() {
  state_ = ContextState::CLOSED;

  audioPlayer_->stop();
  audioPlayer_->cleanup();
  nodeManager_->cleanup();
}

bool AudioContext::resume() {
  if (isClosed()) {
    return false;
  }

  if (isRunning()) {
    return true;
  }

  if (!playerHasBeenStarted_) {
    if (audioPlayer_->start()) {
      playerHasBeenStarted_ = true;
      state_ = ContextState::RUNNING;
      return true;
    }

    return false;
  }

  if (audioPlayer_->resume()) {
    state_ = ContextState::RUNNING;
    return true;
  }

  return false;
}

bool AudioContext::suspend() {
  if (isClosed()) {
    return false;
  }

  if (isSuspended()) {
    return true;
  }

  audioPlayer_->suspend();

  state_ = ContextState::SUSPENDED;
  return true;
}

std::function<void(std::shared_ptr<AudioBus>, int)>
AudioContext::renderAudio() {
  return [this](const std::shared_ptr<AudioBus> &data, int frames) {
    destination_->renderAudio(data, frames);
  };
}

bool AudioContext::isDriverRunning() const {
  return audioPlayer_->isRunning();
}

} // namespace audioapi
