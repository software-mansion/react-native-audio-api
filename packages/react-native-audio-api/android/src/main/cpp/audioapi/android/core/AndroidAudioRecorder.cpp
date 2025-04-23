#include <audioapi/android/core/AndroidAudioRecorder.h>
#include <audioapi/core/Constants.h>
#include <audioapi/utils/AudioArray.h>
#include <audioapi/utils/AudioBus.h>

namespace audioapi {

AndroidAudioRecorder::AndroidAudioRecorder(
    float sampleRate,
    int bufferLength,
    const std::function<void(std::shared_ptr<AudioBus>, int, double)>
        &onAudioReady)
    : sampleRate_(sampleRate),
      bufferLength_(bufferLength),
      onAudioReady_(onAudioReady) {
  AudioStreamBuilder builder;
  builder.setSharingMode(SharingMode::Exclusive)
      ->setDirection(Direction::Input)
      ->setFormat(AudioFormat::Float)
      ->setFormatConversionAllowed(true)
      ->setPerformanceMode(PerformanceMode::None)
      ->setChannelCount(1)
      ->setSampleRateConversionQuality(SampleRateConversionQuality::Medium)
      ->setDataCallback(this)
      ->setSampleRate(static_cast<int>(sampleRate))
      ->setFramesPerDataCallback(bufferLength)
      ->openStream(mStream_);
}

AndroidAudioRecorder::~AndroidAudioRecorder() {
  if (mStream_) {
    mStream_->requestStop();
    mStream_->close();
    mStream_.reset();
  }
}

void AndroidAudioRecorder::start() {
  if (mStream_) {
    mStream_->requestStart();
  }
}

void AndroidAudioRecorder::stop() {
  if (mStream_) {
    mStream_->requestStop();
  }
}

DataCallbackResult AndroidAudioRecorder::onAudioReady(
    oboe::AudioStream *oboeStream,
    void *audioData,
    int32_t numFrames) {
  auto buffer = static_cast<float *>(audioData);

  auto bus = std::make_shared<AudioBus>(bufferLength_, 1, sampleRate_);
  memcpy(bus->getChannel(0)->getData(), buffer, numFrames * sizeof(float));
  auto when = static_cast<double>(
      oboeStream->getTimestamp(CLOCK_MONOTONIC).value().timestamp);

  onAudioReady_(bus, numFrames, when);

  return DataCallbackResult::Continue;
}

} // namespace audioapi
