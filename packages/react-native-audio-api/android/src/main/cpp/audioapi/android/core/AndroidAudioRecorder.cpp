#include <audioapi/android/core/AndroidAudioRecorder.h>
#include <audioapi/core/Constants.h>
#include <audioapi/core/sources/RecorderAdapterNode.h>
#include <audioapi/events/AudioEventHandlerRegistry.h>
#include <audioapi/utils/AudioArray.h>
#include <audioapi/utils/AudioBus.h>
#include <audioapi/utils/CircularAudioArray.h>
#include <audioapi/utils/CircularOverflowableAudioArray.h>

namespace audioapi {

AndroidAudioRecorder::AndroidAudioRecorder(
    float sampleRate,
    int bufferLength,
    const std::shared_ptr<AudioEventHandlerRegistry> &audioEventHandlerRegistry)
    : AudioRecorder(sampleRate, bufferLength, audioEventHandlerRegistry) {
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
  if (isRunning_.load()) {
    return;
  }

  if (mStream_) {
    mStream_->requestStart();
  }

  isRunning_.store(true);
}

void AndroidAudioRecorder::stop() {
  if (!isRunning_.load()) {
    return;
  }

  isRunning_.store(false);

  if (mStream_) {
    mStream_->requestStop();
  }

  sendRemainingData();
}

DataCallbackResult AndroidAudioRecorder::onAudioReady(
    oboe::AudioStream *oboeStream,
    void *audioData,
    int32_t numFrames) {
  if (isRunning_.load()) {
    auto *inputChannel = static_cast<float *>(audioData);
    writeToBuffers(inputChannel, numFrames);
  }

  while (circularBuffer_->getNumberOfAvailableFrames() >= bufferLength_) {
    auto bus = std::make_shared<AudioBus>(bufferLength_, 1, sampleRate_);
    auto *outputChannel = bus->getChannel(0)->getData();

    circularBuffer_->pop_front(outputChannel, bufferLength_);
    auto when = static_cast<double>(
        oboeStream->getTimestamp(CLOCK_MONOTONIC).value().timestamp);

    invokeOnAudioReadyCallback(bus, bufferLength_, when);
  }

  return DataCallbackResult::Continue;
}

} // namespace audioapi
