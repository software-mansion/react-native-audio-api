#include <android/log.h>
#include <audioapi/android/core/AudioPlayer.h>
#include <audioapi/core/AudioContext.h>
#include <audioapi/core/utils/Constants.h>
#include <audioapi/utils/AudioArray.hpp>

#include <jni.h>

#include <algorithm>
#include <memory>

namespace audioapi {

AudioPlayer::AudioPlayer(
    const std::function<void(std::shared_ptr<DSPAudioBuffer>, int)> &renderAudio,
    float sampleRate,
    int channelCount)
    : renderAudio_(renderAudio),
      sampleRate_(sampleRate),
      channelCount_(channelCount),
      isRunning_(false) {}

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
  isInitialized_ = true;
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
    lastCallbackFrameCount_.store(0, std::memory_order_release);
    lastOutputLatencySeconds_.store(0.0, std::memory_order_release);
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
  isInitialized_ = false;

  if (mStream_ != nullptr) {
    mStream_->close();
    mStream_.reset();
  }
}

bool AudioPlayer::isRunning() const {
  return mStream_ && mStream_->getState() == oboe::StreamState::Started &&
      isRunning_.load(std::memory_order_acquire);
}

DataCallbackResult
AudioPlayer::onAudioReady(AudioStream *oboeStream, void *audioData, int32_t numFrames) {
  if (!isInitialized_) {
    return DataCallbackResult::Continue;
  }

  if (numFrames > 0) {
    lastCallbackFrameCount_.store(numFrames, std::memory_order_release);

    // Sample at callback start (minimum of the output latency sawtooth).
    const auto latencyResult = oboeStream->calculateLatencyMillis();
    if (latencyResult) {
      lastOutputLatencySeconds_.store(latencyResult.value() / 1000.0, std::memory_order_release);
    }
  }

  auto *buffer = static_cast<float *>(audioData);
  int processedFrames = 0;

  while (processedFrames < numFrames) {
    auto framesToProcess = std::min(numFrames - processedFrames, RENDER_QUANTUM_SIZE);

    if (isRunning_.load(std::memory_order_acquire)) {
      renderAudio_(buffer_, framesToProcess);
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
  if (error == oboe::Result::ErrorDisconnected) {
    cleanup();
    if (openAudioStream()) {
      isInitialized_ = true;
      resume();
    }
  }
}

double AudioPlayer::getBaseLatency() const {
  if (mStream_ == nullptr || !isInitialized_ || !isRunning()) {
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
  if (mStream_ == nullptr || !isInitialized_ || !isRunning()) {
    return 0.0;
  }
  return lastOutputLatencySeconds_.load(std::memory_order_acquire);
}
} // namespace audioapi
