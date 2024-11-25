#include "AudioBus.h"
#include "AudioArray.h"
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

void OscillatorNode::setPeriodicWave(
    const std::shared_ptr<PeriodicWave> &periodicWave) {
  periodicWave_ = periodicWave;
  type_ = OscillatorType::CUSTOM;
}

void OscillatorNode::processNode(AudioBus* processingBus, int framesToProcess) {
  if (!isPlaying()) {
    processingBus->zero();
    return;
  }

  double time = context_->getCurrentTime();
  double deltaTime = 1.0 / context_->getSampleRate();

  for (int i = 0; i < framesToProcess; i += 1) {
    auto detuneRatio =
      std::pow(2.0f, detuneParam_->getValueAtTime(time) / 1200.0f);
    auto detunedFrequency =
        round(frequencyParam_->getValueAtTime(time) * detuneRatio);
    auto phaseIncrement = detunedFrequency * periodicWave_->getScale();

    float sample =
        periodicWave_->getSample(detunedFrequency, phase_, phaseIncrement);

    for (int j = 0; j < processingBus->getNumberOfChannels(); j += 1) {
      (*processingBus->getChannel(j))[i] = sample;
    }

    phase_ += phaseIncrement;
    phase_ -=
        floor(
            phase_ / static_cast<float>(periodicWave_->getPeriodicWaveSize())) *
        static_cast<float>(periodicWave_->getPeriodicWaveSize());

    time += deltaTime;
  }
}

} // namespace audioapi
