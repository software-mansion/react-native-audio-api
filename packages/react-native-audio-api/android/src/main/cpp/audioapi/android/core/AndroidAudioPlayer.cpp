#include <android/log.h>
#include <audioapi/android/core/AndroidAudioPlayer.h>
#include <audioapi/core/AudioContext.h>
#include <audioapi/core/utils/Constants.h>
#include <audioapi/core/utils/CurrentRenderScope.h>
#include <audioapi/utils/AudioArray.hpp>
#include <audioapi/utils/Macros.h>

#include <jni.h>

#include <algorithm>
#include <cstddef>
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

} // namespace

AndroidAudioPlayer::AndroidAudioPlayer(
    const std::function<void(DSPAudioBuffer *, int)> &renderAudio,
    float sampleRate,
    int channelCount,
    std::atomic<uint32_t> &currentRenders,
    std::weak_ptr<AudioContext> context,
    std::mutex *driverMutex,
    AudioContextLatencyHint latencyHint,
    AndroidOutputProfile outputProfile)
    : AudioPlayer(
          renderAudio,
          sampleRate,
          channelCount,
          currentRenders,
          std::move(context),
          driverMutex,
          latencyHint),
      outputProfile_(outputProfile) {}

bool AndroidAudioPlayer::openAudioStreamLocked() {
  AudioStreamBuilder builder;

  builder.setSharingMode(SharingMode::Exclusive)
      ->setFormat(AudioFormat::Float)
      ->setFormatConversionAllowed(true)
      ->setPerformanceMode(performanceModeFor(latencyHint_))
      ->setChannelCount(channelCount_)
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

  isInitialized_.store(true, std::memory_order_release);
  return true;
}

bool AndroidAudioPlayer::rebuildStreamLocked() {
  cleanupLocked();
  return openAudioStreamLocked();
}

bool AndroidAudioPlayer::startStreamLocked() {
  const bool started = mStream_ != nullptr && mStream_->requestStart() == oboe::Result::OK;
  isRunning_.store(started, std::memory_order_release);
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
    isRunning_.store(false, std::memory_order_release);
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
    isRunning_.store(false, std::memory_order_release);
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
      isRunning_.load(std::memory_order_acquire);
}

DataCallbackResult
AndroidAudioPlayer::onAudioReady(AudioStream *oboeStream, void *audioData, int32_t numFrames) {
  if (!isInitialized_.load(std::memory_order_acquire)) {
    return DataCallbackResult::Continue;
  }

  if (numFrames > 0) {
    lastCallbackFrameCount_.store(numFrames, std::memory_order_release);
  }

  const CurrentRenderScope renderScope(currentRenders_);

  auto *buffer = static_cast<float *>(audioData);
  int processedFrames = 0;

  while (processedFrames < numFrames) {
    auto framesToProcess = std::min(numFrames - processedFrames, RENDER_QUANTUM_SIZE);

    if (isRunning_.load(std::memory_order_acquire)) {
      renderNormalizedQuantum(framesToProcess);
    } else {
      renderBuffer_->zero();
    }

    float *destination = buffer + (static_cast<ptrdiff_t>(processedFrames * channelCount_));

    renderBuffer_->interleaveTo(destination, framesToProcess);
    processedFrames += framesToProcess;
  }

  return DataCallbackResult::Continue;
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
  const bool wasRunning = isRunning_.load(std::memory_order_acquire);

  // Best effort rebuild; a suspended context keeps the rebuilt stream paused until resume().
  if (error == oboe::Result::ErrorDisconnected && rebuildStreamLocked() &&
      (!wasRunning || startStreamLocked())) {
    return;
  }

  isRunning_.store(false, std::memory_order_release);

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

double AndroidAudioPlayer::getOutputLatency() const {
  std::scoped_lock lock(streamMutex_);
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
} // namespace audioapi
