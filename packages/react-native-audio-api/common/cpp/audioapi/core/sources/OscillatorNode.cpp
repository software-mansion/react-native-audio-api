#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/sources/OscillatorNode.h>
#include <audioapi/core/utils/Constants.h>
#include <audioapi/dsp/AudioUtils.hpp>
#include <audioapi/types/NodeOptions.h>
#include <audioapi/utils/AudioArray.hpp>

#include <memory>

namespace audioapi {

OscillatorNode::OscillatorNode(
    const std::shared_ptr<BaseAudioContext> &context,
    const OscillatorOptions &options)
    : AudioScheduledSourceNode(context, options), type_(options.type) {
  frequencyParam_ = std::make_shared<AudioParam>(
      options.frequency, -getNyquistFrequency(), getNyquistFrequency(), context);
  detuneParam_ = std::make_shared<AudioParam>(
      options.detune,
      -static_cast<float>(OCTAVE_RANGE) * LOG2_MOST_POSITIVE_SINGLE_FLOAT,
      static_cast<float>(OCTAVE_RANGE) * LOG2_MOST_POSITIVE_SINGLE_FLOAT,
      context);
  if (options.periodicWave) {
    periodicWave_ = options.periodicWave;
  } else {
    periodicWave_ = context->getBasicWaveForm(type_);
  }
}

std::shared_ptr<AudioParam> OscillatorNode::getFrequencyParam() const {
  return frequencyParam_;
}

std::shared_ptr<AudioParam> OscillatorNode::getDetuneParam() const {
  return detuneParam_;
}

void OscillatorNode::setType(OscillatorType type) {
  if (std::shared_ptr<BaseAudioContext> context = context_.lock()) {
    type_ = type;
    periodicWave_ = context->getBasicWaveForm(type_);
  }
}

void OscillatorNode::setPeriodicWave(const std::shared_ptr<PeriodicWave> &periodicWave) {
  periodicWave_ = periodicWave;
  type_ = OscillatorType::CUSTOM;
}

void OscillatorNode::processNode(int framesToProcess) {
  size_t startOffset = 0;
  size_t offsetLength = 0;

  std::shared_ptr<BaseAudioContext> context = context_.lock();
  if (context == nullptr) {
    audioBuffer_->zero();
    return;
  }

  updatePlaybackInfo(
      audioBuffer_,
      framesToProcess,
      startOffset,
      offsetLength,
      context->getSampleRate(),
      context->getCurrentSampleFrame());

  if (!isPlaying() && !isStopScheduled()) {
    audioBuffer_->zero();
    return;
  }

  auto time =
      context->getCurrentTime() + static_cast<double>(startOffset) / context->getSampleRate();
  auto detuneSpan = detuneParam_->processARateParam(framesToProcess, time)->getChannel(0)->span();
  auto freqSpan = frequencyParam_->processARateParam(framesToProcess, time)->getChannel(0)->span();

  const auto tableSize = static_cast<float>(periodicWave_->getPeriodicWaveSize());
  const auto tableScale = periodicWave_->getScale();
  const auto numChannels = audioBuffer_->getNumberOfChannels();

  auto channelSpan = audioBuffer_->getChannel(0)->span();
  float currentPhase = phase_;

  for (size_t i = startOffset; i < offsetLength; i += 1) {
    auto detuneRatio = detuneSpan[i] == 0 ? 1.0f : exp2f(detuneSpan[i] * CENTS_TO_RATIO);
    auto detunedFrequency = freqSpan[i] * detuneRatio;
    auto phaseIncrement = detunedFrequency * tableScale;

    channelSpan[i] = periodicWave_->getSample(detunedFrequency, currentPhase, phaseIncrement);

    currentPhase += phaseIncrement;

    if (currentPhase >= tableSize) {
      currentPhase -= tableSize;
    } else if (currentPhase < 0.0f) {
      currentPhase += tableSize;
    }
  }

  phase_ = currentPhase;

  for (size_t ch = 1; ch < numChannels; ch += 1) {
    audioBuffer_->getChannel(ch)->copy(*audioBuffer_->getChannel(0));
  }
  handleStopScheduled();
}

} // namespace audioapi
