#include <cassert>

#include "AudioArray.h"
#include "AudioBus.h"
#include "AudioContext.h"
#include "AudioPlayer.h"
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

  sampleRate_ = static_cast<float>(mStream_->getSampleRate());
  mBus_ = std::make_shared<AudioBus>(
      RENDER_QUANTUM_SIZE, CHANNEL_COUNT, sampleRate_);
  isInitialized_ = true;
}

AudioPlayer::AudioPlayer(
    const std::function<void(AudioBus *, int)> &renderAudio,
    float sampleRate)
    : renderAudio_(renderAudio) {
  AudioStreamBuilder builder;

  builder.setSharingMode(SharingMode::Exclusive)
      ->setFormat(AudioFormat::Float)
      ->setFormatConversionAllowed(true)
      ->setPerformanceMode(PerformanceMode::LowLatency)
      ->setChannelCount(CHANNEL_COUNT)
      ->setSampleRateConversionQuality(SampleRateConversionQuality::Medium)
      ->setDataCallback(this)
      ->setSampleRate(static_cast<int>(sampleRate))
      ->openStream(mStream_);

  sampleRate_ = sampleRate;
  mBus_ = std::make_shared<AudioBus>(
      RENDER_QUANTUM_SIZE, CHANNEL_COUNT, sampleRate_);
  isInitialized_ = true;
}

float AudioPlayer::getSampleRate() const {
  return sampleRate_;
}

void AudioPlayer::start() {
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
    renderAudio_(mBus_.get(), framesToProcess);

    // MIXING
    // TODO: optimize this with SIMD?
    for (int i = 0; i < framesToProcess; i++) {
      for (int channel = 0; channel < CHANNEL_COUNT; channel += 1) {
        buffer[(processedFrames + i) * CHANNEL_COUNT + channel] =
            mBus_->getChannel(channel)->getData()[i];
      }
    }

    processedFrames += framesToProcess;
  }

  return DataCallbackResult::Continue;
}

} // namespace audioapi
