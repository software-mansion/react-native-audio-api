#include "NodeAudioPlayer.h"

#include <audioapi/core/utils/Constants.h>

#include <algorithm>
#include <chrono>

namespace audioapi {

NodeAudioPlayer::NodeAudioPlayer(
    const std::function<void(DSPAudioBuffer *, int)> &renderAudio,
    float sampleRate,
    int channelCount)
    : renderAudio_(renderAudio),
      buffer_(std::make_shared<DSPAudioBuffer>(RENDER_QUANTUM_SIZE, channelCount, sampleRate)),
      sampleRate_(sampleRate),
      channelCount_(channelCount) {}

NodeAudioPlayer::~NodeAudioPlayer() {
  stop();
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

  if (worker_.joinable()) {
    worker_.join();
  }
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
  isInitialized_.store(false, std::memory_order_release);
}

bool NodeAudioPlayer::isRunning() const {
  return isRunning_.load(std::memory_order_acquire);
}

void NodeAudioPlayer::run() {
  const auto sampleRate = std::max(sampleRate_, 1.0f);
  const auto quantumDuration = std::chrono::microseconds(static_cast<int64_t>(
      (static_cast<double>(RENDER_QUANTUM_SIZE) * 1'000'000.0) / sampleRate));

  while (!shouldStop_.load(std::memory_order_acquire)) {
    if (isPaused_.load(std::memory_order_acquire)) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
      continue;
    }

    buffer_->zero();
    renderAudio_(buffer_.get(), RENDER_QUANTUM_SIZE);
    std::this_thread::sleep_for(quantumDuration);
  }
}

} // namespace audioapi
