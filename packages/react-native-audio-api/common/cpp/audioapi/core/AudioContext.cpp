#ifdef ANDROID
#include <audioapi/android/core/AudioPlayer.h>
#else
#include <audioapi/ios/core/IOSAudioPlayer.h>
#endif

#include <audioapi/system/AudioManager.h>

#include <audioapi/core/AudioContext.h>
#include <audioapi/core/destinations/AudioDestinationNode.h>
#include <audioapi/core/utils/AudioDecoder.h>
#include <audioapi/core/utils/AudioNodeManager.h>

namespace audioapi {
AudioContext::AudioContext() : BaseAudioContext() {
  auto audioManager = AudioManager::getInstance();
#ifdef ANDROID
  audioPlayer_ = std::make_shared<AudioPlayer>(this->renderAudio());
#else
  audioPlayer_ = std::make_shared<IOSAudioPlayer>(
      audioManager->getIOSManagerBridge(), this->renderAudio());
#endif

  sampleRate_ = audioPlayer_->getSampleRate();
  audioDecoder_ = std::make_shared<AudioDecoder>(sampleRate_);

  audioPlayer_->start();
}

AudioContext::AudioContext(float sampleRate) : BaseAudioContext() {
  auto audioManager = AudioManager::getInstance();
#ifdef ANDROID
  audioPlayer_ = std::make_shared<AudioPlayer>(this->renderAudio(), sampleRate);
#else
  audioPlayer_ = std::make_shared<IOSAudioPlayer>(
      audioManager->getIOSManagerBridge(), this->renderAudio(), sampleRate);
#endif

  sampleRate_ = audioPlayer_->getSampleRate();
  audioDecoder_ = std::make_shared<AudioDecoder>(sampleRate_);

  audioPlayer_->start();
}

AudioContext::~AudioContext() {
  if (!isClosed()) {
    close();
  }
}

void AudioContext::close() {
  state_ = ContextState::CLOSED;
  audioPlayer_->stop();

  nodeManager_->cleanup();
}

bool AudioContext::resume() {
  if (isClosed()) {
    return false;
  }

  state_ = ContextState::RUNNING;
  audioPlayer_->resume();
  return true;
}

bool AudioContext::suspend() {
  if (isClosed()) {
    return false;
  }

  state_ = ContextState::SUSPENDED;
  audioPlayer_->suspend();
  return true;
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
