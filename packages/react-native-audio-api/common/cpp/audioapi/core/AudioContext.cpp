#ifdef ANDROID
#include <audioapi/android/core/AudioPlayer.h>
#else
#include <audioapi/ios/core/IOSAudioPlayer.h>
#endif

#include <audioapi/core/AudioContext.h>
#include <audioapi/core/destinations/AudioDestinationNode.h>
#include <audioapi/core/sources/MediaElementAudioSourceNode.h>
#include <audioapi/core/utils/AudioGraphManager.h>
#include <memory>

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
      this->renderAudio(),
      getSampleRate(),
      destination_->getChannelCount(),
      &driverMutex_,
      std::static_pointer_cast<AudioContext>(shared_from_this()));
  audioPlayer_->openAudioStream();
#else
  audioPlayer_ = std::make_shared<IOSAudioPlayer>(
      this->renderAudio(), getSampleRate(), destination_->getChannelCount());
#endif
}

bool AudioContext::tryStartDriver() {
  if (getState() == ContextState::CLOSED) {
    return false;
  }

  if (isInitialized_.load(std::memory_order_acquire)) {
    return false;
  }

  if (audioPlayer_->start()) {
    isInitialized_.store(true, std::memory_order_release);
    setState(ContextState::RUNNING);
    return true;
  }

  return false;
}

void AudioContext::close() {
  std::scoped_lock lock(driverMutex_);
  setState(ContextState::CLOSED);

  audioPlayer_->stop();
  audioPlayer_->cleanup();
  getGraphManager()->cleanup();
}

bool AudioContext::resume() {
  std::scoped_lock lock(driverMutex_);

  if (getState() == ContextState::CLOSED) {
    return false;
  }

  if (getState() == ContextState::RUNNING) {
    return true;
  }

  if (isInitialized_.load(std::memory_order_acquire) && audioPlayer_->resume()) {
    setState(ContextState::RUNNING);
    return true;
  }

  return tryStartDriver();
}

bool AudioContext::suspend() {
  std::scoped_lock lock(driverMutex_);

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
  std::scoped_lock lock(driverMutex_);
  return tryStartDriver();
}

std::function<void(std::shared_ptr<DSPAudioBuffer>, int)> AudioContext::renderAudio() {
  return [this](const std::shared_ptr<DSPAudioBuffer> &data, int frames) {
    destination_->renderAudio(data, frames);
  };
}

bool AudioContext::isDriverRunning() const {
  return audioPlayer_->isRunning();
}

std::shared_ptr<MediaElementAudioSourceNode> AudioContext::createMediaElementSource(
    const std::shared_ptr<AudioFileSourceNode> &fileSource) {

  if (fileSource->isRoutedThroughMediaElement()) {
    return nullptr;
  }

  auto mediaElementSource = std::make_shared<MediaElementAudioSourceNode>(
      shared_from_this(),
      fileSource,
      MediaElementAudioSourceOptions(static_cast<int>(fileSource->getChannelCount())));

  fileSource->bindMediaElementSource(mediaElementSource->getBindingId());

  graphManager_->addProcessingNode(mediaElementSource);
  return mediaElementSource;
}

} // namespace audioapi
