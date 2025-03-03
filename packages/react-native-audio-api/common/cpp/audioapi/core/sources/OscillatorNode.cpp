#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/sources/OscillatorNode.h>
#include <audioapi/utils/AudioArray.h>
#include <audioapi/utils/AudioBus.h>

namespace audioapi {

OscillatorNode::OscillatorNode(BaseAudioContext *context)
    : AudioScheduledSourceNode(context) {
  frequencyParam_ = std::make_shared<AudioParam>(
      444.0, -context_->getNyquistFrequency(), context_->getNyquistFrequency());
  detuneParam_ = std::make_shared<AudioParam>(0.0, -MAX_DETUNE, MAX_DETUNE);
  type_ = OscillatorType::SINE;
  periodicWave_ = context_->getBasicWaveForm(type_);
  isInitialized_ = true;
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

void OscillatorNode::processNode(
    const std::shared_ptr<AudioBus> &processingBus,
    int framesToProcess) {
  size_t startOffset = 0;
  size_t offsetLength = 0;

  updatePlaybackInfo(processingBus, framesToProcess, startOffset, offsetLength);

  if (!isPlaying()) {
    processingBus->zero();
    return;
  }

  auto deltaTime = 1.0 / context_->getSampleRate();
  auto time =
      context_->getCurrentTime() + static_cast<double>(startOffset) * deltaTime;

  for (size_t i = startOffset; i < offsetLength; i += 1) {
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

  handleStopScheduled();
}

} // namespace audioapi
