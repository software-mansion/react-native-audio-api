#include <android/log.h>
#include <audioapi/android/core/AudioPlayer.h>
#include <audioapi/core/AudioContext.h>
#include <audioapi/core/utils/Constants.h>
#include <audioapi/core/utils/CurrentRenderScope.h>
#include <audioapi/utils/AudioArray.hpp>

#include <jni.h>

#include <algorithm>
#include <memory>
#include <mutex>

namespace audioapi {

AudioPlayer::AudioPlayer(
    const std::function<void(DSPAudioBuffer *, int)> &renderAudio,
    float sampleRate,
    int channelCount,
    std::mutex *driverMutex,
    const std::shared_ptr<AudioContext> &context,
    std::atomic<uint32_t> &currentRenders)
    : renderAudio_(renderAudio),
      currentRenders_(currentRenders),
      sampleRate_(sampleRate),
      channelCount_(channelCount),
      isRunning_(false),
      driverMutex_(driverMutex),
      context_(context) {}

bool AudioPlayer::openAudioStream() {
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

  buffer_ = std::make_shared<DSPAudioBuffer>(RENDER_QUANTUM_SIZE, channelCount_, sampleRate_);
  isInitialized_.store(true, std::memory_order_release);
  return true;
}

bool AudioPlayer::start() {
  if (mStream_ != nullptr) {
    auto result = mStream_->requestStart() == oboe::Result::OK;
    isRunning_.store(result, std::memory_order_release);
    return result;
  }

  return false;
}

void AudioPlayer::stop() {
  if (mStream_ != nullptr) {
    isRunning_.store(false, std::memory_order_release);
    mStream_->requestStop();
  }
}

bool AudioPlayer::resume() {
  if (isRunning()) {
    return true;
  }

  if (mStream_ != nullptr) {
    auto result = mStream_->requestStart() == oboe::Result::OK;
    isRunning_.store(result, std::memory_order_release);
    return result;
  }

  return false;
}

void AudioPlayer::suspend() {
  if (mStream_ != nullptr) {
    isRunning_.store(false, std::memory_order_release);
    mStream_->requestPause();
  }
}

void AudioPlayer::cleanup() {
  isInitialized_.store(false, std::memory_order_release);

  if (mStream_ != nullptr) {
    mStream_->close();
    mStream_.reset();
  }
}

bool AudioPlayer::isRunning() const {
  return mStream_ != nullptr && mStream_->getState() == oboe::StreamState::Started &&
      isRunning_.load(std::memory_order_acquire);
}

DataCallbackResult
AudioPlayer::onAudioReady(AudioStream *oboeStream, void *audioData, int32_t numFrames) {
  if (!isInitialized_.load(std::memory_order_acquire)) {
    return DataCallbackResult::Continue;
  }

  const CurrentRenderScope renderScope(currentRenders_);

  auto *buffer = static_cast<float *>(audioData);
  int processedFrames = 0;

  while (processedFrames < numFrames) {
    auto framesToProcess = std::min(numFrames - processedFrames, RENDER_QUANTUM_SIZE);

    if (isRunning_.load(std::memory_order_acquire)) {
      renderAudio_(buffer_.get(), framesToProcess);
    } else {
      buffer_->zero();
    }

    float *destination = buffer + (processedFrames * channelCount_);

    buffer_->interleaveTo(destination, framesToProcess);
    processedFrames += framesToProcess;
  }

  return DataCallbackResult::Continue;
}

void AudioPlayer::onErrorAfterClose(oboe::AudioStream *stream, oboe::Result error) {
  if (error != oboe::Result::ErrorDisconnected || driverMutex_ == nullptr) {
    return;
  }

  // Serialize with start()/resume()/suspend()/close() on the JS / promise-pool threads.
  std::scoped_lock lock(*driverMutex_);

  auto context = context_.lock();
  if (context == nullptr || context->isClosed()) {
    return;
  }

  // The error thread is detached and can arrive late: if close() or a concurrent
  // recovery already replaced the stream, don't tear down the healthy new stream.
  if (stream != mStream_.get()) {
    return;
  }

  cleanup();
  if (openAudioStream()) {
    resume();
  }
}
} // namespace audioapi
