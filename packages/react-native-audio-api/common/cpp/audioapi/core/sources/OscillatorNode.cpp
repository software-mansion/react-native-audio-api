#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/sources/OscillatorNode.h>
#include <audioapi/dsp/AudioUtils.h>
#include <audioapi/utils/AudioArray.h>
#include <audioapi/utils/AudioBus.h>
#include <memory>
#include <string>

namespace audioapi {

OscillatorNode::OscillatorNode(std::shared_ptr<BaseAudioContext> context)
    : AudioScheduledSourceNode(context),
      frequencyParam_(
          std::make_shared<AudioParam>(
              444.0,
              -context->getNyquistFrequency(),
              context->getNyquistFrequency(),
              context)),
      detuneParam_(
          std::make_shared<AudioParam>(
              0.0,
              -1200 * LOG2_MOST_POSITIVE_SINGLE_FLOAT,
              1200 * LOG2_MOST_POSITIVE_SINGLE_FLOAT,
              context)),
      type_(OscillatorType::SINE),
      periodicWave_(context->getBasicWaveForm(type_)) {
  audioBus_ = std::make_shared<AudioBus>(RENDER_QUANTUM_SIZE, 1, context->getSampleRate());
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
  if (std::shared_ptr<BaseAudioContext> context = context_.lock()) {
    type_ = OscillatorNode::fromString(type);
    periodicWave_ = context->getBasicWaveForm(type_);
  }
}

void OscillatorNode::setPeriodicWave(const std::shared_ptr<PeriodicWave> &periodicWave) {
  periodicWave_ = periodicWave;
  type_ = OscillatorType::CUSTOM;
}

std::shared_ptr<AudioBus> OscillatorNode::processNode(
    const std::shared_ptr<AudioBus> &processingBus,
    int framesToProcess) {
  size_t startOffset = 0;
  size_t offsetLength = 0;

  std::shared_ptr<BaseAudioContext> context = context_.lock();
  if (context == nullptr) {
    processingBus->zero();
    return processingBus;
  }

  updatePlaybackInfo(processingBus, framesToProcess, startOffset, offsetLength, context->getSampleRate(), context->getCurrentSampleFrame());

  if (!isPlaying() && !isStopScheduled()) {
    processingBus->zero();
    return processingBus;
  }

  auto time =
      context->getCurrentTime() + static_cast<double>(startOffset) * 1.0 / context->getSampleRate();
  auto detuneParamValues = detuneParam_->processARateParam(framesToProcess, time);
  auto frequencyParamValues = frequencyParam_->processARateParam(framesToProcess, time);

  for (size_t i = startOffset; i < offsetLength; i += 1) {
    auto detuneRatio = std::pow(2.0f, detuneParamValues->getChannel(0)->getData()[i] / 1200.0f);
    auto detunedFrequency = frequencyParamValues->getChannel(0)->getData()[i] * detuneRatio;
    auto phaseIncrement = detunedFrequency * periodicWave_->getScale();

    float sample = periodicWave_->getSample(detunedFrequency, phase_, phaseIncrement);

    for (int j = 0; j < processingBus->getNumberOfChannels(); j += 1) {
      (*processingBus->getChannel(j))[i] = sample;
    }

    phase_ += phaseIncrement;
    phase_ -= floor(phase_ / static_cast<float>(periodicWave_->getPeriodicWaveSize())) *
        static_cast<float>(periodicWave_->getPeriodicWaveSize());
  }

  handleStopScheduled();

  return processingBus;
}

} // namespace audioapi
