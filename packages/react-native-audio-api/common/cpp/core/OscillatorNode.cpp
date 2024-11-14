#include "OscillatorNode.h"
#include "BaseAudioContext.h"

namespace audioapi {

OscillatorNode::OscillatorNode(BaseAudioContext *context)
    : AudioScheduledSourceNode(context) {
  frequencyParam_ = std::make_shared<AudioParam>(
      context, 444.0, -NYQUIST_FREQUENCY, NYQUIST_FREQUENCY);
  detuneParam_ =
      std::make_shared<AudioParam>(context, 0.0, -MAX_DETUNE, MAX_DETUNE);
  type_ = OscillatorType::SINE;
  periodicWave_ = context_->getBasicWaveForm(type_);
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
  periodicWave_ = context_->getBasicWaveForm(type_);
}

bool OscillatorNode::processAudio(float *audioData, int32_t numFrames) {
  if (!isPlaying_) {
    return false;
  } else {
    auto time = context_->getCurrentTime();
    auto deltaTime = 1.0 / context_->getSampleRate();

    for (int i = 0; i < numFrames; ++i) {
      auto detuneRatio =
          std::pow(2.0f, detuneParam_->getValueAtTime(time) / 1200.0f);
      auto detunedFrequency =
          round(frequencyParam_->getValueAtTime(time) * detuneRatio);
      auto phaseIncrement = detunedFrequency * periodicWave_->getRateScale();

      float sample = periodicWave_->getWaveTableElement(
          detunedFrequency, phase_, phaseIncrement);

      for (int j = 0; j < channelCount_; j++) {
        audioData[i * channelCount_ + j] = sample;
      }

      phase_ += phaseIncrement;

      if (phase_ >= static_cast<float>(periodicWave_->getPeriodicWaveSize())) {
        phase_ -= static_cast<float>(periodicWave_->getPeriodicWaveSize());
      }

        time += deltaTime;
    }

    return true;
  }
}
} // namespace audioapi
