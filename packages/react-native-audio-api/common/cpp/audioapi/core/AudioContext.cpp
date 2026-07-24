#ifdef RN_AUDIO_API_NODE
#include "NodeAudioPlayer.h"
#elif defined(ANDROID)
#include <audioapi/android/core/AudioPlayer.h>
#else
#include <audioapi/ios/core/IOSAudioPlayer.h>
#endif

#include <audioapi/core/AudioContext.h>
#include <audioapi/core/destinations/AudioDestinationNode.h>
#include <memory>
#include <thread>

namespace audioapi {
AudioContext::AudioContext(
    float sampleRate,
    const std::shared_ptr<IAudioEventHandlerRegistry> &audioEventHandlerRegistry)
    : BaseAudioContext(sampleRate, audioEventHandlerRegistry), isInitialized_(false) {
  // Context starts SUSPENDED with no audio-thread consumer. Let the producer
  // drain Channel A itself until start()/resume() hands draining to the
  // audio callback (same pattern as OfflineAudioContext before rendering).
  getGraph()->setProducerSelfDrain(true);
}

AudioContext::~AudioContext() {
  if (getState() != ContextState::CLOSED) {
    close();
  }
}

void AudioContext::initialize(const AudioDestinationNode *destination) {
  BaseAudioContext::initialize(destination);
#ifdef RN_AUDIO_API_NODE
  audioPlayer_ = std::make_shared<NodeAudioPlayer>(
      [this](DSPAudioBuffer *buf, int n) { processGraph(buf, n); },
      getSampleRate(),
      destination_->getChannelCount());
#elif defined(ANDROID)
  audioPlayer_ = std::make_shared<AudioPlayer>(
      [this](DSPAudioBuffer *buf, int n) { processGraph(buf, n); },
      getSampleRate(),
      destination_->getChannelCount(),
      &driverMutex_,
      std::static_pointer_cast<AudioContext>(shared_from_this()),
      currentRenders_);
#else
  audioPlayer_ = std::make_shared<IOSAudioPlayer>(
      [this](DSPAudioBuffer *buf, int n) { processGraph(buf, n); },
      getSampleRate(),
      destination_->getChannelCount(),
      currentRenders_);
#endif
}

bool AudioContext::tryStartDriver() {
  assertDriverMutexHeld();

  if (getState() == ContextState::CLOSED) {
    return false;
  }

  if (isInitialized_.load(std::memory_order_acquire)) {
    return false;
  }

  // Flush while we are still the sole consumer, then hand the channel to the
  // audio callback. flushing first avoids blocking forever if the bounded
  // channel was full when self-drain is turned off.
  getGraph()->processEvents();
  getGraph()->setProducerSelfDrain(false);

  if (audioPlayer_->start()) {
    isInitialized_.store(true, std::memory_order_release);
    setState(ContextState::RUNNING);
    return true;
  }

  getGraph()->setProducerSelfDrain(true);
  return false;
}

void AudioContext::close() {
  std::scoped_lock lock(driverMutex_);
  setState(ContextState::CLOSED);

  audioPlayer_->stop();
  waitForRenderQuiescence();
  // No audio-thread consumer after stop; allow producer self-drain for any
  // remaining graph mutations (and flush events already queued).
  getGraph()->setProducerSelfDrain(true);
  getGraph()->processEvents();
  processAudioEvents();
  audioPlayer_->cleanup();
}

bool AudioContext::resume() {
  std::scoped_lock lock(driverMutex_);

  if (getState() == ContextState::CLOSED) {
    return false;
  }

  if (getState() == ContextState::RUNNING) {
    return true;
  }

  if (isInitialized_.load(std::memory_order_acquire)) {
    getGraph()->processEvents();
    getGraph()->setProducerSelfDrain(false);
    if (audioPlayer_->resume()) {
      setState(ContextState::RUNNING);
      return true;
    }
    getGraph()->setProducerSelfDrain(true);
    return false;
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
  waitForRenderQuiescence();
  // Audio callback is no longer the consumer; enable self-drain so graph
  // mutations while suspended cannot fill the bounded channel and block.
  getGraph()->setProducerSelfDrain(true);
  getGraph()->processEvents();
  processAudioEvents();
  setState(ContextState::SUSPENDED);
  return true;
}

bool AudioContext::start() {
  if (isInitialized_.load(std::memory_order_acquire)) {
    return false;
  }

  assertDriverMutexHeld();
  return tryStartDriver();
}

void AudioContext::waitForRenderQuiescence() const {
  assertDriverMutexHeld();
  while (currentRenders_.load(std::memory_order_acquire) != 0) {
    std::this_thread::yield();
  }
}

bool AudioContext::isDriverRunning() const {
  return audioPlayer_->isRunning();
}

double AudioContext::getBaseLatency() const {
  if (audioPlayer_ == nullptr) {
    return 0.0;
  }

  return audioPlayer_->getBaseLatency();
}

double AudioContext::getOutputLatency() const {
  if (audioPlayer_ == nullptr) {
    return 0.0;
  }

  return audioPlayer_->getOutputLatency();
}

} // namespace audioapi
