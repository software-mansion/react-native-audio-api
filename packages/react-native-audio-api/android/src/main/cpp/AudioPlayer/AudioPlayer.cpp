
#include "AudioPlayer.h"
#include "AudioArray.h"
#include "AudioBus.h"
#include "AudioContext.h"
#include "Constants.h"

namespace audioapi {

AudioPlayer::AudioPlayer(
    const std::function<void(AudioBus *, int)> &renderAudio)
    : renderAudio_(renderAudio) {
  AudioStreamBuilder builder;

  builder.setSharingMode(SharingMode::Exclusive)
      ->setFormat(AudioFormat::Float)
      ->setFormatConversionAllowed(true)
      ->setPerformanceMode(PerformanceMode::LowLatency)
      ->setChannelCount(CHANNEL_COUNT)
      ->setSampleRateConversionQuality(SampleRateConversionQuality::Medium)
      ->setDataCallback(this)
      ->openStream(mStream_);

  mBus_ = std::make_shared<AudioBus>(
      getSampleRate(), getBufferSizeInFrames(), CHANNEL_COUNT);
}

int AudioPlayer::getSampleRate() const {
  return mStream_->getSampleRate();
}

int AudioPlayer::getBufferSizeInFrames() const {
  return mStream_->getBufferSizeInFrames();
}

void AudioPlayer::start() {
  if (mStream_) {
    mStream_->requestStart();
  }
}

void AudioPlayer::stop() {
  if (mStream_) {
    mStream_->requestStop();
    mStream_->close();
    mStream_.reset();
  }
}

DataCallbackResult AudioPlayer::onAudioReady(
    AudioStream *oboeStream,
    void *audioData,
    int32_t numFrames) {
  auto buffer = static_cast<float *>(audioData);

  renderAudio_(mBus_.get(), numFrames);

  // TODO: optimize this with SIMD?
  for (int32_t i = 0; i < numFrames; i += 1) {
    for (int channel = 0; channel < CHANNEL_COUNT; channel += 1) {
      buffer[i * CHANNEL_COUNT + channel] =
          mBus_->getChannel(channel)->getData()[i];
    }
  }

  return DataCallbackResult::Continue;
}

} // namespace audioapi
