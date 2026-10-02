#include <android/log.h>
#include <audioapi/android/core/AudioPlayer.h>
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

namespace audioapi {

bool AudioPlayer::openAudioStream() {
  std::scoped_lock lock(streamMutex_);
  AudioStreamBuilder builder;

  builder.setSharingMode(SharingMode::Exclusive)
      ->setFormat(AudioFormat::Float)
      ->setFormatConversionAllowed(true)
      ->setPerformanceMode(PerformanceMode::LowLatency)
      ->setChannelCount(channelCount_)
      ->setSampleRateConversionQuality(SampleRateConversionQuality::Medium)
      ->setFramesPerDataCallback(RENDER_QUANTUM_SIZE)
      ->setDataCallback(shared_from_this())
      ->setErrorCallback(shared_from_this())
      ->setSampleRate(static_cast<int>(sampleRate_));

  auto result = builder.openStream(mStream_);
  if (result != oboe::Result::OK || mStream_ == nullptr) {
    __android_log_print(
        ANDROID_LOG_ERROR, "AudioPlayer", "Failed to open stream: %s", oboe::convertToText(result));
    return false;
  }

  isInitialized_.store(true, std::memory_order_release);
  return true;
}

bool AudioPlayer::rebuildStream() {
  cleanup();
  return openAudioStream();
}

bool AudioPlayer::start() {
  std::scoped_lock lock(streamMutex_);

  if ((!isInitialized_.load(std::memory_order_acquire)) && (!openAudioStream())) {
    return false;
  }

  if (mStream_ != nullptr) {
    auto result = mStream_->requestStart() == oboe::Result::OK;
    isRunning_.store(result, std::memory_order_release);
    return result;
  }

  return false;
}

void AudioPlayer::stop() {
  std::scoped_lock lock(streamMutex_);
  if (mStream_ != nullptr) {
    isRunning_.store(false, std::memory_order_release);
    lastCallbackFrameCount_.store(0, std::memory_order_release);
    mStream_->requestStop();
  }
}

bool AudioPlayer::resume() {
  std::scoped_lock lock(streamMutex_);
  if (isRunning()) {
    return true;
  }

  // The stream may have been dropped by onErrorAfterClose while suspended.
  if ((!isInitialized_.load(std::memory_order_acquire)) && (!openAudioStream())) {
    return false;
  }

  if (mStream_ != nullptr) {
    auto result = mStream_->requestStart() == oboe::Result::OK;
    isRunning_.store(result, std::memory_order_release);
    return result;
  }

  return false;
}

void AudioPlayer::suspend() {
  std::scoped_lock lock(streamMutex_);
  if (mStream_ != nullptr) {
    isRunning_.store(false, std::memory_order_release);
    mStream_->requestPause();
  }
}

void AudioPlayer::cleanup() {
  std::scoped_lock lock(streamMutex_);
  isInitialized_.store(false, std::memory_order_release);

  if (mStream_ != nullptr) {
    mStream_->close();
    mStream_.reset();
  }
}

bool AudioPlayer::isRunning() const {
  std::scoped_lock lock(streamMutex_);
  return mStream_ != nullptr && mStream_->getState() == oboe::StreamState::Started &&
      isRunning_.load(std::memory_order_acquire);
}

DataCallbackResult
AudioPlayer::onAudioReady(AudioStream *oboeStream, void *audioData, int32_t numFrames) {
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

void AudioPlayer::onErrorAfterClose(oboe::AudioStream *stream, oboe::Result error) {
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
  std::scoped_lock lock(*driverMutex_, streamMutex_);

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
  if (error == oboe::Result::ErrorDisconnected && rebuildStream()) {
    if (!wasRunning) {
      return;
    }
    if (mStream_->requestStart() == oboe::Result::OK) {
      isRunning_.store(true, std::memory_order_release);
      return;
    }
  }

  isRunning_.store(false, std::memory_order_release);

  if (!wasRunning) {
    // Nothing was playing, so there is no failure to report: drop the dead stream
    // and let resume() open a new one.
    cleanup();
    return;
  }

  context->onStreamFail();
}

double AudioPlayer::getBaseLatency() const {
  std::scoped_lock lock(streamMutex_);
  if (mStream_ == nullptr || !isInitialized_.load(std::memory_order_acquire) || !isRunning()) {
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

double AudioPlayer::getOutputLatency() const {
  std::scoped_lock lock(streamMutex_);
  if (mStream_ == nullptr || !isInitialized_.load(std::memory_order_acquire) || !isRunning()) {
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
