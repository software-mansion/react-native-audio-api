#include <android/log.h>
#include <audioapi/android/core/AudioPlayer.h>
#include <audioapi/core/AudioContext.h>
#include <audioapi/core/utils/Constants.h>
#include <audioapi/utils/AudioArray.h>
#include <audioapi/utils/AudioBuffer.h>
#include <jni.h>

#include <algorithm>
#include <memory>

namespace audioapi {

AudioPlayer::AudioPlayer(
    const std::function<void(std::shared_ptr<AudioBuffer>, int)> &renderAudio,
    float sampleRate,
    int channelCount)
    : renderAudio_(renderAudio),
      sampleRate_(sampleRate),
      channelCount_(channelCount),
      isRunning_(false) {
  isInitialized_ = openAudioStream();
}

bool AudioPlayer::openAudioStream() {
  AudioStreamBuilder builder;

  builder.setSharingMode(SharingMode::Exclusive)
      ->setFormat(AudioFormat::Float)
      ->setFormatConversionAllowed(true)
      ->setPerformanceMode(PerformanceMode::None)
      ->setChannelCount(channelCount_)
      ->setSampleRateConversionQuality(SampleRateConversionQuality::Medium)
      ->setDataCallback(this)
      ->setSampleRate(static_cast<int>(sampleRate_))
      ->setErrorCallback(this);

  auto result = builder.openStream(mStream_);
  if (result != oboe::Result::OK || mStream_ == nullptr) {
    __android_log_print(
        ANDROID_LOG_ERROR, "AudioPlayer", "Failed to open stream: %s", oboe::convertToText(result));
    return false;
  }

  buffer_ = std::make_shared<AudioBuffer>(RENDER_QUANTUM_SIZE, channelCount_, sampleRate_);
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

  auto buffer = static_cast<float *>(audioData);
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
} // namespace audioapi
