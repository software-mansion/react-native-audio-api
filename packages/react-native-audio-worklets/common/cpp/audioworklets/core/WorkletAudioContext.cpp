#include <audioworklets/core/WorkletAudioContext.h>

#include <audioapi/core/destinations/AudioDestinationNode.h>
#include <audioapi/core/types/ContextState.h>

#ifdef ANDROID
#include <fbjni/fbjni.h>
#endif

#include <chrono>
#include <memory>
#include <thread>

namespace audioworklets {

WorkletAudioContext::WorkletAudioContext(
    float sampleRate,
    const std::shared_ptr<audioapi::IAudioEventHandlerRegistry> &audioEventHandlerRegistry)
    : audioapi::BaseAudioContext(sampleRate, audioEventHandlerRegistry) {}

WorkletAudioContext::~WorkletAudioContext() {
  if (getState() != audioapi::ContextState::CLOSED) {
    close();
  }
}

void WorkletAudioContext::initialize(const audioapi::AudioDestinationNode *destination) {
  audioapi::BaseAudioContext::initialize(destination);
  buffer_ = std::make_shared<audioapi::DSPAudioBuffer>(
      audioapi::RENDER_QUANTUM_SIZE, destination->getChannelCount(), getSampleRate());
}

void WorkletAudioContext::close() {
  std::unique_lock lock(driverMutex_);

  if (getState() == audioapi::ContextState::CLOSED) {
    return;
  }

  setState(audioapi::ContextState::CLOSED);
  shouldStop_ = true;

  lock.unlock();

  if (worker_.joinable()) {
    worker_.join();
  }

  lock.lock();
  processAudioEvents();
  isInitialized_ = false;
}

bool WorkletAudioContext::resume() {
  std::scoped_lock lock(driverMutex_);

  if (getState() == audioapi::ContextState::CLOSED) {
    return false;
  }

  if (getState() == audioapi::ContextState::RUNNING) {
    return true;
  }

  if (!isInitialized_) {
    shouldStop_ = false;
    worker_ = std::thread(&WorkletAudioContext::run, this);
    isInitialized_ = true;
  }

  setState(audioapi::ContextState::RUNNING);
  return true;
}

bool WorkletAudioContext::suspend() {
  std::scoped_lock lock(driverMutex_);

  if (getState() == audioapi::ContextState::CLOSED) {
    return false;
  }

  if (getState() == audioapi::ContextState::SUSPENDED) {
    return true;
  }

  setState(audioapi::ContextState::SUSPENDED);
  processAudioEvents();
  return true;
}

bool WorkletAudioContext::isDriverRunning() const {
  return state_.load(std::memory_order_acquire) == audioapi::ContextState::RUNNING;
}

void WorkletAudioContext::run() {
  const auto quantumDuration = std::chrono::microseconds(
      static_cast<int64_t>(
          (static_cast<double>(audioapi::RENDER_QUANTUM_SIZE) * 1'000'000.0) / getSampleRate()));

  // Deadline pacing keeps the soft clock realtime-aligned; sleeping a full
  // quantum after process drifts behind and fills the recorder adapter ring.
  auto nextDeadline = std::chrono::steady_clock::now();
#ifdef ANDROID
  // worklet nodes touch jni, so we need to attach this thread to the jniEnv
  auto threadScope = facebook::jni::ThreadScope();
#endif

  while (true) {
    bool rendered = false;

    {
      std::scoped_lock lock(driverMutex_);

      if (shouldStop_) {
        break;
      }

      if (state_.load(std::memory_order_acquire) == audioapi::ContextState::RUNNING &&
          buffer_ != nullptr) {
        buffer_->zero();
        processGraph(buffer_.get(), audioapi::RENDER_QUANTUM_SIZE);
        rendered = true;
      }
    }

    if (!rendered) {
      nextDeadline = std::chrono::steady_clock::now();
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
      continue;
    }

    nextDeadline += quantumDuration;
    const auto now = std::chrono::steady_clock::now();
    if (now < nextDeadline) {
      std::this_thread::sleep_until(nextDeadline);
    } else {
      // Late: snap forward instead of catch-up spinning (would drain the ring).
      nextDeadline = now;
    }
  }
}

} // namespace audioworklets
