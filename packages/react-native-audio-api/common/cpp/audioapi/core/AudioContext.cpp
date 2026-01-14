#ifdef ANDROID
#include <audioapi/android/core/AudioPlayer.h>
#else
#include <audioapi/ios/core/IOSAudioPlayer.h>
#endif

#include <audioapi/core/AudioContext.h>
#include <audioapi/core/destinations/AudioDestinationNode.h>
#include <audioapi/core/utils/AudioNodeManager.h>
#include <memory>
#include <string>

namespace audioapi {
AudioContext::AudioContext(
    float sampleRate,
    const std::shared_ptr<IAudioEventHandlerRegistry> &audioEventHandlerRegistry,
    const RuntimeRegistry &runtimeRegistry)
    : BaseAudioContext(audioEventHandlerRegistry, runtimeRegistry), isInitialized_(false) {
  sampleRate_ = sampleRate;
  state_ = ContextState::SUSPENDED;
}

AudioContext::~AudioContext() {
  if (!isClosed()) {
    close();
  }
}

void AudioContext::initialize() {
  BaseAudioContext::initialize();
#ifdef ANDROID
  audioPlayer_ = std::make_shared<AudioPlayer>(
      this->renderAudio(), sampleRate_, destination_->getChannelCount());
#else
  audioPlayer_ = std::make_shared<IOSAudioPlayer>(
      this->renderAudio(), sampleRate_, destination_->getChannelCount());
#endif
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

  if (isInitialized_ && audioPlayer_->resume()) {
    state_ = ContextState::RUNNING;
    return true;
  }

  return start();
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

bool AudioContext::start() {
  if (isClosed()) {
    return false;
  }

  if (!isInitialized_ && audioPlayer_->start()) {
    isInitialized_ = true;
    state_ = ContextState::RUNNING;

    return true;
  }

  return false;
}

std::function<void(std::shared_ptr<AudioBus>, int)> AudioContext::renderAudio() {
  return [this](const std::shared_ptr<AudioBus> &data, int frames) {
    destination_->renderAudio(data, frames);
  };
}

bool AudioContext::isDriverRunning() const {
  return audioPlayer_->isRunning();
}

} // namespace audioapi
