#include <android/log.h>
#include <audioapi/android/core/AudioPlayer.h>
#include <audioapi/core/AudioContext.h>
#include <audioapi/core/utils/Constants.h>
#include <audioapi/utils/AudioArray.h>
#include <audioapi/utils/AudioBus.h>
#include <jni.h>

#include <algorithm>
#include <memory>

namespace audioapi {

AudioPlayer::AudioPlayer(
    const std::function<void(std::shared_ptr<AudioBus>, int)> &renderAudio,
    float sampleRate,
    int channelCount)
    : renderAudio_(renderAudio), sampleRate_(sampleRate), channelCount_(channelCount) {
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

  mBus_ = std::make_shared<AudioBus>(RENDER_QUANTUM_SIZE, channelCount_, sampleRate_);
  return true;
}

bool AudioPlayer::start() {
  if (mStream_) {
    auto result = mStream_->requestStart();
    return result == oboe::Result::OK;
  }

  return false;
}

void AudioPlayer::stop() {
  if (mStream_) {
    mStream_->requestStop();
  }
}

bool AudioPlayer::resume() {
  if (mStream_) {
    auto result = mStream_->requestStart();
    return result == oboe::Result::OK;
  }

  return false;
}

void AudioPlayer::suspend() {
  if (mStream_) {
    mStream_->requestPause();
  }
}

void AudioPlayer::cleanup() {
  isInitialized_ = false;

  if (mStream_) {
    mStream_->close();
    mStream_.reset();
  }
}

bool AudioPlayer::isRunning() const {
  return mStream_ && mStream_->getState() == oboe::StreamState::Started;
}

DataCallbackResult
AudioPlayer::onAudioReady(AudioStream *oboeStream, void *audioData, int32_t numFrames) {
  if (!isInitialized_) {
    return DataCallbackResult::Continue;
  }

  auto buffer = static_cast<float *>(audioData);
  int processedFrames = 0;

  assert(buffer != nullptr);

  while (processedFrames < numFrames) {
    int framesToProcess = std::min(numFrames - processedFrames, RENDER_QUANTUM_SIZE);
    renderAudio_(mBus_, framesToProcess);

    // TODO: optimize this with SIMD?
    for (int i = 0; i < framesToProcess; i++) {
      for (int channel = 0; channel < channelCount_; channel += 1) {
        buffer[(processedFrames + i) * channelCount_ + channel] =
            mBus_->getChannel(channel)->getData()[i];
      }
    }

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
