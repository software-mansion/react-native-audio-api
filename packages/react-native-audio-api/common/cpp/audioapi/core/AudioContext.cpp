#ifdef ANDROID
#include <audioapi/android/core/AudioPlayer.h>
#else
#include <audioapi/ios/core/IOSAudioPlayer.h>
#endif

#include <audioapi/core/AudioContext.h>
#include <audioapi/core/destinations/AudioDestinationNode.h>
#include <audioapi/core/sources/AudioFileSourceNode.h>
#include <audioapi/core/sources/MediaElementAudioSourceNode.h>
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

void AudioContext::initialize(const AudioDestinationNode *destination) {
  BaseAudioContext::initialize(destination);
#ifdef ANDROID
  audioPlayer_ = std::make_shared<AudioPlayer>(
      [this](DSPAudioBuffer *buf, int n) { processGraph(buf, n); },
      getSampleRate(),
      destination_->getChannelCount());
#else
  audioPlayer_ = std::make_shared<IOSAudioPlayer>(
      [this](DSPAudioBuffer *buf, int n) { processGraph(buf, n); },
      getSampleRate(),
      destination_->getChannelCount());
#endif
}

void AudioContext::close() {
  setState(ContextState::CLOSED);

  audioPlayer_->stop();
  audioPlayer_->cleanup();
}

bool AudioContext::resume() {
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

  if (!isInitialized_.load(std::memory_order_acquire) && audioPlayer_->start()) {
    isInitialized_.store(true, std::memory_order_release);
    setState(ContextState::RUNNING);

    return true;
  }

  return false;
}

bool AudioContext::isDriverRunning() const {
  return audioPlayer_->isRunning();
}

} // namespace audioapi
