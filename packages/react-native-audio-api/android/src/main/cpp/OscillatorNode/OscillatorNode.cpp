#include "OscillatorNode.h"

namespace audioapi {

OscillatorNode::OscillatorNode() : AudioScheduledSourceNode() {
  // TODO add Constants class
  frequencyParam_ = std::make_shared<AudioParam>(444.0, -22050.0, 22050.0);
  detuneParam_ = std::make_shared<AudioParam>(0.0, -1200.0, 1200.0);

  AudioStreamBuilder builder;
  builder.setSharingMode(SharingMode::Shared)
      ->setPerformanceMode(PerformanceMode::LowLatency)
      ->setChannelCount(channelCount_)
      ->setSampleRate(sampleRate)
      ->setSampleRateConversionQuality(SampleRateConversionQuality::None)
      ->setFormat(AudioFormat::Float)
      ->setDataCallback(this)
      ->openStream(mStream);
}

std::shared_ptr<AudioParam> OscillatorNode::getFrequencyParam() const {
  return frequencyParam_;
}

std::shared_ptr<AudioParam> OscillatorNode::getDetuneParam() const {
  return detuneParam_;
}

std::string OscillatorNode::getType() {
  return type_;
}

void OscillatorNode::setType(const std::string &type) {
  type_ = type;
}

DataCallbackResult OscillatorNode::onAudioReady(
    AudioStream *oboeStream,
    void *audioData,
    int32_t numFrames) {
  auto *floatData = (float *)audioData;
  for (int i = 0; i < numFrames; ++i) {
    float sampleValue = 1 * sinf(phase_);
    for (int j = 0; j < channelCount_; j++) {
      floatData[i * channelCount_ + j] = sampleValue;
    }
    phase_ += frequencyParam_->getValue() * 2 * (float)M_PI / (float)sampleRate;
    if (phase_ >= 2 * M_PI)
      phase_ -= 2 * M_PI;
  }

  process(oboeStream, audioData, numFrames, channelCount_);

  return DataCallbackResult::Continue;
}

} // namespace audioapi
