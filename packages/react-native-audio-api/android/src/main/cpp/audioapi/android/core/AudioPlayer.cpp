#include <audioapi/android/core/AudioPlayer.h>
#include <audioapi/core/AudioContext.h>
#include <audioapi/core/Constants.h>
#include <audioapi/utils/AudioArray.h>
#include <audioapi/utils/AudioBus.h>

namespace audioapi {

AudioPlayer::AudioPlayer(
    const std::function<void(std::shared_ptr<AudioBus>, int)> &renderAudio,
    float sampleRate)
    : renderAudio_(renderAudio), channelCount_(2) {
  AudioStreamBuilder builder;

  builder.setSharingMode(SharingMode::Exclusive)
      ->setFormat(AudioFormat::Float)
      ->setFormatConversionAllowed(true)
      ->setPerformanceMode(PerformanceMode::None)
      ->setChannelCount(2)
      ->setSampleRateConversionQuality(SampleRateConversionQuality::Medium)
      ->setDataCallback(this)
      ->setSampleRate(static_cast<int>(sampleRate))
      ->openStream(mStream_);

  sampleRate_ = sampleRate;
  mBus_ = std::make_shared<AudioBus>(RENDER_QUANTUM_SIZE, 2, sampleRate_);
  isInitialized_ = true;
}

void AudioPlayer::start() {
  if (mStream_) {
    mStream_->requestStart();
  }
}

void AudioPlayer::resume() {
  if (mStream_) {
    mStream_->requestStart();
  }
}

void AudioPlayer::stop() {
  isInitialized_ = false;

  if (mStream_) {
    mStream_->requestStop();
    mStream_->close();
    mStream_.reset();
  }
}

void AudioPlayer::pause() {
  if (mStream_) {
    mStream_->requestPause();
  }
}

DataCallbackResult AudioPlayer::onAudioReady(
    AudioStream *oboeStream,
    void *audioData,
    int32_t numFrames) {
  if (!isInitialized_) {
    return DataCallbackResult::Continue;
  }

  auto buffer = static_cast<float *>(audioData);
  int processedFrames = 0;

  assert(buffer != nullptr);

  while (processedFrames < numFrames) {
    int framesToProcess =
        std::min(numFrames - processedFrames, RENDER_QUANTUM_SIZE);
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

} // namespace audioapi
