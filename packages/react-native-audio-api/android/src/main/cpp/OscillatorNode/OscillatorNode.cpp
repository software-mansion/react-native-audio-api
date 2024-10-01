#include "OscillatorNode.h"

namespace audioapi {

OscillatorNode::OscillatorNode() : AudioScheduledSourceNode() {
  AudioStreamBuilder builder;
  builder.setSharingMode(SharingMode::Shared)
      ->setPerformanceMode(PerformanceMode::LowLatency)
      ->setChannelCount(channelCount_)
      ->setSampleRate(sampleRate)
      ->setSampleRateConversionQuality(SampleRateConversionQuality::Medium)
      ->setFormat(AudioFormat::Float)
      ->setDataCallback(this)
      ->openStream(mStream);
}

DataCallbackResult OscillatorNode::onAudioReady(
    oboe::AudioStream *oboeStream,
    void *audioData,
    int32_t numFrames) {
  auto *floatData = (float *)audioData;
  for (int i = 0; i < numFrames; ++i) {
    float sampleValue = gain_ * sinf(phase_);
    for (int j = 0; j < channelCount_; j++) {
      floatData[i * channelCount_ + j] = sampleValue;
    }
    phase_ += frequency_ * 2 * M_PI / (double)sampleRate;
    if (phase_ >= 2 * M_PI)
      phase_ -= 2 * M_PI;
  }
  return oboe::DataCallbackResult::Continue;
}

} // namespace audioapi
