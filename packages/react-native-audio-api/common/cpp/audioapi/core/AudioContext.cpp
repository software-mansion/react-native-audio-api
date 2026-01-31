#ifdef ANDROID
#include <audioapi/android/core/AudioPlayer.h>
#else
#include <audioapi/ios/core/IOSAudioPlayer.h>
#endif

#include <audioapi/core/AudioContext.h>
#include <audioapi/core/destinations/AudioDestinationNode.h>
#include <audioapi/core/utils/AudioGraphManager.h>
#include <memory>
#include <string>

namespace audioapi {
AudioContext::AudioContext(
    float sampleRate,
    const std::shared_ptr<IAudioEventHandlerRegistry> &audioEventHandlerRegistry,
    const RuntimeRegistry &runtimeRegistry)
    : BaseAudioContext(sampleRate, audioEventHandlerRegistry, runtimeRegistry),
    isInitialized_(false) {}

AudioContext::~AudioContext() {
  if (getState() != ContextState::CLOSED) {
    close();
  }
}

void AudioContext::initialize() {
  BaseAudioContext::initialize();
#ifdef ANDROID
  audioPlayer_ = std::make_shared<AudioPlayer>(
      this->renderAudio(), getSampleRate(), destination_->getChannelCount());
#else
  audioPlayer_ = std::make_shared<IOSAudioPlayer>(
      this->renderAudio(), getSampleRate(), destination_->getChannelCount());
#endif
}

void AudioContext::close() {
  setState(ContextState::CLOSED);

  audioPlayer_->stop();
  audioPlayer_->cleanup();
  getGraphManager()->cleanup();
}

bool AudioContext::resume() {
  if (getState() == ContextState::CLOSED) {
    return false;
  }

  if (getState() == ContextState::RUNNING) {
    return true;
  }

  if (isInitialized_ && audioPlayer_->resume()) {
    setState(ContextState::RUNNING);
    return true;
  }

  return start();
}

bool AudioContext::suspend() {
  if (getState() == ContextState::CLOSED) {
    return false;
  }

  if (getState() == ContextState::SUSPENDED) {
    return true;
  }

  audioPlayer_->suspend();

  setState(ContextState::SUSPENDED);
  return true;
}

bool AudioContext::start() {
  if (getState() == ContextState::CLOSED) {
    return false;
  }

  if (!isInitialized_ && audioPlayer_->start()) {
    isInitialized_ = true;
    setState(ContextState::RUNNING);

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
