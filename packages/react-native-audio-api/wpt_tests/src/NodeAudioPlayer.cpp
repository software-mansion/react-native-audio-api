#include "NodeAudioPlayer.h"

#include <audioapi/core/utils/Constants.h>

#include <algorithm>
#include <chrono>
#include <memory>
#include <optional>
#include <utility>

namespace audioapi {

NodeAudioPlayer::~NodeAudioPlayer() {
  // Always join here — do not gate on isInitialized_. After cleanup()/stop()
  // the flag may already be false while a joinable worker still exists, and
  // process.exit() finalizers must not leave orphan audio threads.
  terminateWorker();
}

void NodeAudioPlayer::terminateWorker() {
  shouldStop_.store(true, std::memory_order_release);
  isPaused_.store(true, std::memory_order_release);
  isRunning_.store(false, std::memory_order_release);

  if (worker_.joinable()) {
    worker_.join();
  }

  isInitialized_.store(false, std::memory_order_release);
}

bool NodeAudioPlayer::start() {
  if (isInitialized_.load(std::memory_order_acquire)) {
    return false;
  }

  shouldStop_.store(false, std::memory_order_release);
  isPaused_.store(false, std::memory_order_release);
  isRunning_.store(true, std::memory_order_release);
  isInitialized_.store(true, std::memory_order_release);

  worker_ = std::thread(&NodeAudioPlayer::run, this);
  return true;
}

void NodeAudioPlayer::stop() {
  if (!isInitialized_.load(std::memory_order_acquire)) {
    return;
  }

  shouldStop_.store(true, std::memory_order_release);
  isPaused_.store(true, std::memory_order_release);
  isRunning_.store(false, std::memory_order_release);

  if (!worker_.joinable()) {
    return;
  }

  // `AudioContext::close()` may run as an SPSC control message on this worker
  // (inside `processGraph` → `processAudioEvents`). Joining ourselves throws
  // `resource_deadlock_would_occur`. Signal exit and let a non-worker caller
  // (destructor / later `stop()` from the JS thread) join.
  if (std::this_thread::get_id() == worker_.get_id()) {
    return;
  }

  worker_.join();
}

bool NodeAudioPlayer::resume() {
  if (!isInitialized_.load(std::memory_order_acquire)) {
    return false;
  }

  if (isRunning_.load(std::memory_order_acquire)) {
    return true;
  }

  isPaused_.store(false, std::memory_order_release);
  isRunning_.store(true, std::memory_order_release);
  return true;
}

void NodeAudioPlayer::suspend() {
  if (!isInitialized_.load(std::memory_order_acquire)) {
    return;
  }

  isPaused_.store(true, std::memory_order_release);
  isRunning_.store(false, std::memory_order_release);
}

void NodeAudioPlayer::cleanup() {
  stop();
  // Only clear the initialized flag once the worker has been joined. If `stop()`
  // ran on the worker, keep `isInitialized_` so a later JS-thread `stop()` /
  // destructor can still join.
  if (!worker_.joinable()) {
    isInitialized_.store(false, std::memory_order_release);
  }
}

bool NodeAudioPlayer::isRunning() const {
  return isRunning_.load(std::memory_order_acquire);
}

double NodeAudioPlayer::getOutputLatency() const {
  return 0.0;
}

double NodeAudioPlayer::getBaseLatency() const {
  return 0.0;
}

std::optional<OutputPresentation> NodeAudioPlayer::getOutputPresentation() {
  const auto anchor = latestOutputAnchor();
  if (!anchor) {
    return std::nullopt;
  }

  return OutputPresentation{
      .contextFrame = anchor->contextFrame, .presentationTimeNs = anchor->hostTimeNs};
}

void NodeAudioPlayer::run() {
  const auto sampleRate = std::max(sampleRate_, 1.0f);
  const auto quantumDuration = std::chrono::microseconds(static_cast<int64_t>(
      (static_cast<double>(RENDER_QUANTUM_SIZE) * 1'000'000.0) / sampleRate));

  // The virtual device's own clock: when the quantum about to be rendered leaves it. Paced by
  // absolute deadlines so a render's cost does not accumulate into the reported timestamps.
  auto nextQuantumAt = std::chrono::steady_clock::now();

  while (!shouldStop_.load(std::memory_order_acquire)) {
    if (isPaused_.load(std::memory_order_acquire)) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
      // Resume plays on from now, instead of racing to catch up on the silent time.
      nextQuantumAt = std::chrono::steady_clock::now();
      continue;
    }

    // Before the render, so `renderedFrames_` is still the quantum's first frame.
    publishOutputAnchor(
        {.contextFrame = renderedFrames_,
         .hostTimeNs = std::chrono::duration_cast<std::chrono::nanoseconds>(
                           nextQuantumAt.time_since_epoch())
                           .count(),
         .isValid = true});

    renderBuffer_->zero();
    renderNormalizedQuantum(RENDER_QUANTUM_SIZE);

    nextQuantumAt += quantumDuration;
    const auto now = std::chrono::steady_clock::now();
    if (nextQuantumAt <= now) {
      // The render did not keep up; a real device would have underrun here. Drop the backlog
      // rather than busy-rendering to catch up.
      nextQuantumAt = now;
      continue;
    }

    std::this_thread::sleep_until(nextQuantumAt);
  }
}

std::shared_ptr<AudioPlayer> createPlatformAudioPlayer(
    const std::function<void(DSPAudioBuffer *, int)> &renderAudio,
    float sampleRate,
    int channelCount,
    std::atomic<uint32_t> &currentRenders,
    std::weak_ptr<AudioContext> context,
    std::mutex *driverMutex,
    AudioContextLatencyHint latencyHint,
    AndroidOutputProfile /*outputProfile*/) {
  return std::make_shared<NodeAudioPlayer>(
      renderAudio,
      sampleRate,
      channelCount,
      currentRenders,
      std::move(context),
      driverMutex,
      latencyHint);
}

} // namespace audioapi
