#include "OscillatorNode.h"
#include "AudioContext.h"

namespace audioapi {

OscillatorNode::OscillatorNode(AudioContext *context)
    : AudioScheduledSourceNode(context) {
  frequencyParam_ =
      std::make_shared<AudioParam>(context, 444.0, -NYQUIST_FREQUENCY, NYQUIST_FREQUENCY);
  detuneParam_ = std::make_shared<AudioParam>(context, 0.0, -MAX_DETUNE, MAX_DETUNE);

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
  auto *floatData = reinterpret_cast<float *>(audioData);
  for (int i = 0; i < numFrames; ++i) {
    float sampleValue = 1 * sinf(phase_);
    for (int j = 0; j < channelCount_; j++) {
      floatData[i * channelCount_ + j] = sampleValue;
    }
    phase_ += static_cast<float>(
        frequencyParam_->getValueAtTime(context_->getCurrentTime()) * 2 * M_PI /
        sampleRate);
    if (phase_ >= 2 * M_PI)
      phase_ -= 2 * M_PI;
  }

  process(oboeStream, audioData, numFrames, channelCount_);

  return DataCallbackResult::Continue;
}

} // namespace audioapi
