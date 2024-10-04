#include "OscillatorNode.h"
#include "AudioContext.h"

namespace audioapi {

OscillatorNode::OscillatorNode(AudioContext *context)
    : AudioScheduledSourceNode(context) {
  frequencyParam_ = std::make_shared<AudioParam>(
      context, 444.0, -NYQUIST_FREQUENCY, NYQUIST_FREQUENCY);
  detuneParam_ =
      std::make_shared<AudioParam>(context, 0.0, -MAX_DETUNE, MAX_DETUNE);

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
  return OscillatorNode::toString(type_);
}

void OscillatorNode::setType(const std::string &type) {
  type_ = OscillatorNode::fromString(type);
}

DataCallbackResult OscillatorNode::onAudioReady(
    AudioStream *oboeStream,
    void *audioData,
    int32_t numFrames) {
  auto *floatData = reinterpret_cast<float *>(audioData);

  for (int i = 0; i < numFrames; ++i) {
    auto detuneRatio = std::pow(
        2.0f,
        detuneParam_->getValueAtTime(context_->getCurrentTime()) / 1200.0f);
    auto detunedFrequency =
        frequencyParam_->getValueAtTime(context_->getCurrentTime()) *
        detuneRatio;
    auto phaseIncrement =
        static_cast<float>(2 * M_PI * detunedFrequency / sampleRate);
    float value = OscillatorNode::getWaveBufferElement(phase_, type_);

    for (int j = 0; j < channelCount_; j++) {
      floatData[i * channelCount_ + j] = value;
    }
    phase_ += phaseIncrement;
    if (phase_ >= 2 * M_PI)
      phase_ -= 2 * M_PI;
  }

  process(oboeStream, audioData, numFrames, channelCount_);

  return DataCallbackResult::Continue;
}

} // namespace audioapi
