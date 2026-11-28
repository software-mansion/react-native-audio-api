#include <android/log.h>
#include <audioapi/android/core/AndroidAudioPlayer.h>
#include <audioapi/core/AudioContext.h>
#include <audioapi/core/utils/Constants.h>
#include <audioapi/core/utils/CurrentRenderScope.h>
#include <audioapi/utils/AudioArray.hpp>
#include <audioapi/utils/Macros.h>

#include <jni.h>
#include <sched.h>

#include <time.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <utility>

namespace audioapi {

namespace {

PerformanceMode performanceModeFor(AudioContextLatencyHint latencyHint) {
  switch (latencyHint) {
    case AudioContextLatencyHint::INTERACTIVE:
      return PerformanceMode::LowLatency;
    case AudioContextLatencyHint::BALANCED:
      return PerformanceMode::None;
    case AudioContextLatencyHint::PLAYBACK:
      return PerformanceMode::PowerSaving;
  }
  return PerformanceMode::LowLatency;
}

/// Exclusive bypasses the mixer and its effects (echo-cancellation reference included).
SharingMode sharingModeFor(AudioContextLatencyHint latencyHint, AndroidOutputProfile profile) {
  return latencyHint == AudioContextLatencyHint::INTERACTIVE &&
          profile != AndroidOutputProfile::VoiceCommunication
      ? SharingMode::Exclusive
      : SharingMode::Shared;
}

/// About one second of 128-frame callbacks at 48 kHz.
constexpr uint32_t CALLBACKS_BETWEEN_AFFINITY_CHECKS = 375;

} // namespace

AndroidAudioPlayer::AndroidAudioPlayer(
    const std::function<void(DSPAudioBuffer *, int)> &renderAudio,
    float sampleRate,
    std::atomic<uint32_t> &currentRenders,
    std::weak_ptr<AudioContext> context,
    std::mutex *driverMutex,
    AudioContextLatencyHint latencyHint,
    AndroidOutputProfile outputProfile)
    : AudioPlayer(
          renderAudio,
          sampleRate,
          currentRenders,
          std::move(context),
          driverMutex,
          latencyHint),
      outputProfile_(outputProfile) {}

bool AndroidAudioPlayer::openAudioStreamLocked() {
  AudioStreamBuilder builder;

  builder.setSharingMode(sharingModeFor(latencyHint_, outputProfile_))
      ->setFormat(AudioFormat::Float)
      ->setFormatConversionAllowed(true)
      ->setPerformanceMode(performanceModeFor(latencyHint_))
      ->setChannelCount(DEFAULT_CHANNEL_COUNT)
      ->setSampleRateConversionQuality(SampleRateConversionQuality::Medium)
      ->setFramesPerDataCallback(RENDER_QUANTUM_SIZE)
      ->setDataCallback(shared_from_this())
      ->setErrorCallback(shared_from_this())
      ->setSampleRate(static_cast<int>(sampleRate_));

  if (outputProfile_ == AndroidOutputProfile::VoiceCommunication) {
    builder.setUsage(Usage::VoiceCommunication)->setContentType(ContentType::Speech);
  }

  auto result = builder.openStream(mStream_);
  if (result != oboe::Result::OK || mStream_ == nullptr) {
    __android_log_print(
        ANDROID_LOG_ERROR,
        "AndroidAudioPlayer",
        "Failed to open stream: %s",
        oboe::convertToText(result));
    return false;
  }

  renderBuffer_ = std::make_unique<DSPAudioBuffer>(
      RENDER_QUANTUM_SIZE, mStream_->getChannelCount(), sampleRate_);
  mStream_->setPerformanceHintEnabled(true);

  isInitialized_.store(true, std::memory_order_release);
  return true;
}

bool AndroidAudioPlayer::rebuildStreamLocked() {
  cleanupLocked();
  return openAudioStreamLocked();
}

bool AndroidAudioPlayer::startStreamLocked() {
  streamStartedAtNs_ = steadyClockNowNs();
  markStarted();
  const bool started = mStream_ != nullptr && mStream_->requestStart() == oboe::Result::OK;
  isRunning_->store(started, std::memory_order_release);
  return started;
}

bool AndroidAudioPlayer::start() {
  std::scoped_lock lock(streamMutex_);

  if ((!isInitialized_.load(std::memory_order_acquire)) && (!openAudioStreamLocked())) {
    return false;
  }

  return startStreamLocked();
}

void AndroidAudioPlayer::stop() {
  std::scoped_lock lock(streamMutex_);
  if (mStream_ != nullptr) {
    isRunning_->store(false, std::memory_order_release);
    lastCallbackFrameCount_.store(0, std::memory_order_release);
    mStream_->requestStop();
  }
}

bool AndroidAudioPlayer::resume() {
  std::scoped_lock lock(streamMutex_);
  if (isRunningLocked()) {
    return true;
  }

  // The stream may have been dropped by onErrorAfterClose while suspended.
  if (!isInitialized_.load(std::memory_order_acquire) && !openAudioStreamLocked()) {
    return false;
  }

  if (startStreamLocked()) {
    return true;
  }

  // Oboe reports a disconnect only through the callback thread, which a paused
  // stream does not have. Thus it is impossible to observe, such a change.
  // Because of that we attempt to rebuild the stream and start it again, and if that fails, we return false.
  return rebuildStreamLocked() && startStreamLocked();
}

void AndroidAudioPlayer::suspend() {
  std::scoped_lock lock(streamMutex_);
  if (mStream_ != nullptr) {
    isRunning_->store(false, std::memory_order_release);
    mStream_->requestPause();
  }
}

void AndroidAudioPlayer::cleanup() {
  std::scoped_lock lock(streamMutex_);
  cleanupLocked();
}

void AndroidAudioPlayer::cleanupLocked() {
  isInitialized_.store(false, std::memory_order_release);

  if (mStream_ != nullptr) {
    mStream_->close();
    mStream_.reset();
  }
}

bool AndroidAudioPlayer::isRunning() const {
  std::scoped_lock lock(streamMutex_);
  return isRunningLocked();
}

bool AndroidAudioPlayer::isRunningLocked() const {
  return mStream_ != nullptr && mStream_->getState() == oboe::StreamState::Started &&
      isRunning_->load(std::memory_order_acquire);
}

DataCallbackResult
AndroidAudioPlayer::onAudioReady(AudioStream *oboeStream, void *audioData, int32_t numFrames) {
  const CurrentRenderScope renderScope(currentRenders_);

  if (!isInitialized_.load(std::memory_order_acquire)) {
    return DataCallbackResult::Continue;
  }

  if (numFrames > 0) {
    lastCallbackFrameCount_.store(numFrames, std::memory_order_release);
  }

  keepAudioThreadOnOneCore();

  // Oboe counts a device write into getFramesWritten() only after the device callback returns.
  const auto framesWritten = oboeStream->getFramesWritten();
  if (framesWritten != lastFramesWritten_) {
    // Oboe's block adapter holds back less than one block of what we rendered for the previous
    // write. Anything else means the position jumped (new stream, AAudio skipping an underrun).
    const auto framesHeldByAdapter = streamFramesDelivered_ - framesWritten;
    if (framesHeldByAdapter < 0 || framesHeldByAdapter >= RENDER_QUANTUM_SIZE) {
      streamFramesDelivered_ = framesWritten;
    }
    lastFramesWritten_ = framesWritten;
  }

  if (isRunning_->load(std::memory_order_acquire)) {
    publishOutputAnchor(
        {.contextFrame = renderedFrames_,
         .hostTimeNs = steadyClockNowNs(),
         .streamFrame = streamFramesDelivered_,
         .isValid = true});
  }
  streamFramesDelivered_ += numFrames;

  auto *buffer = static_cast<float *>(audioData);
  int processedFrames = 0;

  while (processedFrames < numFrames) {
    auto framesToProcess = std::min(numFrames - processedFrames, RENDER_QUANTUM_SIZE);

    if (isRunning_->load(std::memory_order_acquire)) {
      renderNormalizedQuantum(framesToProcess);
    } else {
      renderBuffer_->zero();
    }

    auto *destination =
        buffer + (static_cast<ptrdiff_t>(processedFrames * renderBuffer_->getNumberOfChannels()));

    renderBuffer_->interleaveTo(destination, framesToProcess);
    processedFrames += framesToProcess;
  }

  return DataCallbackResult::Continue;
}

void AndroidAudioPlayer::keepAudioThreadOnOneCore() {
  if (callbacksUntilAffinityCheck_ > 0) {
    --callbacksUntilAffinityCheck_;
    return;
  }
  callbacksUntilAffinityCheck_ = CALLBACKS_BETWEEN_AFFINITY_CHECKS;

  cpu_set_t current;
  CPU_ZERO(&current);
  if (sched_getaffinity(0, sizeof(current), &current) == 0 && CPU_COUNT(&current) == 1) {
    return;
  }

  cpu_set_t thisCore;
  CPU_ZERO(&thisCore);
  CPU_SET(sched_getcpu(), &thisCore);
  sched_setaffinity(0, sizeof(thisCore), &thisCore);
}

namespace {
struct ReentrancyGuard {
  DELETE_COPY_AND_MOVE(ReentrancyGuard);
  explicit ReentrancyGuard(bool *f) : flag(f) {
    *flag = true;
  }
  ~ReentrancyGuard() {
    *flag = false;
  }

 private:
  bool *flag;
};
} // namespace

void AndroidAudioPlayer::onErrorAfterClose(oboe::AudioStream *stream, oboe::Result error) {
  if (driverMutex_ == nullptr) {
    return;
  }

  switch (error) {
    case oboe::Result::ErrorDisconnected:
    case oboe::Result::ErrorTimeout:
    case oboe::Result::ErrorInternal:
    case oboe::Result::ErrorNoService:
      break;
    default:
      return;
  }

  // Reentrancy guard - prevent recursive calls to onErrorAfterClose.
  static thread_local bool isInsideOnError = false;
  if (isInsideOnError) {
    return;
  }
  ReentrancyGuard guard(&isInsideOnError);

  auto context = context_.lock();
  if (context == nullptr) {
    return;
  }

  // Serialize with start()/resume()/suspend()/close() on the JS / promise-pool threads.
  std::scoped_lock driverLock(*driverMutex_);
  std::unique_lock streamLock(streamMutex_);

  if (context->isClosed()) {
    return;
  }

  // The error thread is detached and can arrive late: if close() or a concurrent
  // recovery already replaced the stream, don't tear down the healthy new stream.
  if (stream != mStream_.get()) {
    return;
  }

  // Check if the stream was expected to be running when the error occurred
  const bool wasRunning = isRunning_->load(std::memory_order_acquire);

  // Best effort rebuild; a suspended context keeps the rebuilt stream paused until resume().
  if (error == oboe::Result::ErrorDisconnected && rebuildStreamLocked() &&
      (!wasRunning || startStreamLocked())) {
    return;
  }

  isRunning_->store(false, std::memory_order_release);

  if (!wasRunning) {
    // Nothing was playing, so there is no failure to report: drop the dead stream
    // and let resume() open a new one.
    cleanupLocked();
    return;
  }

  // onStreamFail() stops the player through the public stop(), which takes streamMutex_.
  streamLock.unlock();
  context->onStreamFail();
}

double AndroidAudioPlayer::getBaseLatency() const {
  std::scoped_lock lock(streamMutex_);
  if (mStream_ == nullptr || !isInitialized_.load(std::memory_order_acquire) ||
      !isRunningLocked()) {
    return 0.0;
  }

  const int32_t callbackFrames = lastCallbackFrameCount_.load(std::memory_order_acquire);
  if (callbackFrames > 0 && sampleRate_ > 0.0f) {
    return static_cast<double>(callbackFrames) / static_cast<double>(sampleRate_);
  }

  const int32_t framesPerBurst = mStream_->getFramesPerBurst();
  if (framesPerBurst > 0) {
    return static_cast<double>(framesPerBurst) / static_cast<double>(sampleRate_);
  }

  return static_cast<double>(RENDER_QUANTUM_SIZE) / static_cast<double>(sampleRate_);
}

std::optional<OutputPresentation> AndroidAudioPlayer::getOutputPresentation() {
  // Held across the anchor read: recovery swaps mStream_ and calls markStarted under it, so a
  // start-count match also means the anchor came from the current mStream_.
  std::scoped_lock lock(streamMutex_);
  const auto anchor = latestOutputAnchor();
  if (!anchor) {
    return std::nullopt;
  }

  // A device timestamp from before the start is AAudio's stale pre-pause report.
  if (isRunningLocked()) {
    const auto presented = mStream_->getTimestamp(CLOCK_MONOTONIC);
    if (presented && presented.value().timestamp >= streamStartedAtNs_) {
      return OutputPresentation{
          .contextFrame = anchor->contextFrame + (presented.value().position - anchor->streamFrame),
          .presentationTimeNs = presented.value().timestamp};
    }
  }

  const auto outputLatencyNs =
      static_cast<std::int64_t>(std::llround(getOutputLatencyLocked() * 1e9));
  return OutputPresentation{
      .contextFrame = anchor->contextFrame,
      .presentationTimeNs = anchor->hostTimeNs + outputLatencyNs};
}

double AndroidAudioPlayer::getOutputLatency() const {
  std::scoped_lock lock(streamMutex_);
  return getOutputLatencyLocked();
}

double AndroidAudioPlayer::getOutputLatencyLocked() const {
  if (mStream_ == nullptr || !isInitialized_.load(std::memory_order_acquire) ||
      !isRunningLocked()) {
    return 0.0;
  }

  double minBaseLatency = 0.0;
  const int32_t callbackFrames = lastCallbackFrameCount_.load(std::memory_order_acquire);
  if (callbackFrames > 0 && sampleRate_ > 0.0f) {
    minBaseLatency = static_cast<double>(callbackFrames) / static_cast<double>(sampleRate_);
  } else if (sampleRate_ > 0.0f) {
    const int32_t framesPerBurst = mStream_->getFramesPerBurst();
    minBaseLatency =
        static_cast<double>(framesPerBurst > 0 ? framesPerBurst : RENDER_QUANTUM_SIZE) /
        static_cast<double>(sampleRate_);
  }

  const auto latencyResult = mStream_->calculateLatencyMillis();
  if (latencyResult) {
    return std::max(latencyResult.value() / 1000.0, minBaseLatency);
  }

  return minBaseLatency;
}
std::shared_ptr<AudioPlayer> createPlatformAudioPlayer(
    const std::function<void(DSPAudioBuffer *, int)> &renderAudio,
    float sampleRate,
    std::atomic<uint32_t> &currentRenders,
    std::weak_ptr<AudioContext> context,
    std::mutex *driverMutex,
    AudioContextLatencyHint latencyHint,
    AndroidOutputProfile outputProfile) {
  return std::make_shared<AndroidAudioPlayer>(
      renderAudio,
      sampleRate,
      currentRenders,
      std::move(context),
      driverMutex,
      latencyHint,
      outputProfile);
}

} // namespace audioapi
