#include <audioapi/types/NodeOptions.h>
#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/sources/OscillatorNode.h>
#include <audioapi/dsp/AudioUtils.hpp>
#include <audioapi/utils/AudioArray.h>
#include <audioapi/utils/AudioBuffer.h>
#include <memory>
#include <string>

namespace audioapi {

OscillatorNode::OscillatorNode(
    const std::shared_ptr<BaseAudioContext> &context,
    const OscillatorOptions &options)
    : AudioScheduledSourceNode(context, options) {
  frequencyParam_ = std::make_shared<AudioParam>(
      options.frequency, -context->getNyquistFrequency(), context->getNyquistFrequency(), context);
  detuneParam_ = std::make_shared<AudioParam>(
      options.detune,
      -1200 * LOG2_MOST_POSITIVE_SINGLE_FLOAT,
      1200 * LOG2_MOST_POSITIVE_SINGLE_FLOAT,
      context);
  type_ = options.type;
  if (options.periodicWave) {
    periodicWave_ = options.periodicWave;
  } else {
    periodicWave_ = context->getBasicWaveForm(type_);
  }

  audioBuffer_ = std::make_shared<AudioBuffer>(RENDER_QUANTUM_SIZE, 1, context->getSampleRate());

  isInitialized_ = true;
}

std::shared_ptr<AudioParam> OscillatorNode::getFrequencyParam() const {
  return frequencyParam_;
}

std::shared_ptr<AudioParam> OscillatorNode::getDetuneParam() const {
  return detuneParam_;
}

OscillatorType OscillatorNode::getType() {
  return type_;
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

std::shared_ptr<AudioBuffer> OscillatorNode::processNode(
    const std::shared_ptr<AudioBuffer> &processingBuffer,
    int framesToProcess) {
  size_t startOffset = 0;
  size_t offsetLength = 0;

  std::shared_ptr<BaseAudioContext> context = context_.lock();
  if (context == nullptr) {
    processingBuffer->zero();
    return processingBuffer;
  }

  updatePlaybackInfo(
      processingBuffer,
      framesToProcess,
      startOffset,
      offsetLength,
      context->getSampleRate(),
      context->getCurrentSampleFrame());

  if (!isPlaying() && !isStopScheduled()) {
    processingBuffer->zero();
    return processingBuffer;
  }

  auto time =
      context->getCurrentTime() + static_cast<double>(startOffset) * 1.0 / context->getSampleRate();
  auto detuneSpan = detuneParam_->processARateParam(framesToProcess, time)->getChannel(0)->span();
  auto freqSpan = frequencyParam_->processARateParam(framesToProcess, time)->getChannel(0)->span();

  const auto tableSize = static_cast<float>(periodicWave_->getPeriodicWaveSize());
  const auto tableScale = periodicWave_->getScale();
  const auto numChannels = processingBuffer->getNumberOfChannels();

  auto finalPhase = phase_;

  for (size_t ch = 0; ch < numChannels; ch += 1) {
    auto channelSpan = processingBuffer->getChannel(ch)->span();
    float currentPhase = phase_;

    for (size_t i = startOffset; i < offsetLength; i += 1) {
      auto detuneRatio = detuneSpan[i] == 0 ? 1.0f : std::pow(2.0f, detuneSpan[i] / 1200.0f);
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

    if (ch == 0) {
      finalPhase = currentPhase;
    }
  }

  phase_ = finalPhase;
  handleStopScheduled();

  return processingBuffer;
}

} // namespace audioapi
