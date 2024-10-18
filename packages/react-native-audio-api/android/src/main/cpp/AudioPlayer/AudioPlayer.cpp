#include "AudioPlayer.h"
#include "AudioContext.h"

namespace audioapi {
AudioPlayer::AudioPlayer(const std::function<void(float *, int)> &renderAudio)
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
}

int AudioPlayer::getSampleRate() const {
  return mStream_->getSampleRate();
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
  renderAudio_(buffer, numFrames);

  return DataCallbackResult::Continue;
}
} // namespace audioapi
