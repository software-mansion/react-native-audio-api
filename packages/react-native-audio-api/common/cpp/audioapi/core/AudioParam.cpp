#include <audioapi/core/AudioParam.h>
#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/utils/param/ParamRenderEventFactory.hpp>
#include <audioapi/dsp/AudioUtils.hpp>
#include <audioapi/dsp/VectorMath.h>
#include <audioapi/utils/AudioArray.hpp>
#include <memory>
#include <utility>

namespace audioapi {

AudioParam::AudioParam(
    float defaultValue,
    float minValue,
    float maxValue,
    const std::shared_ptr<BaseAudioContext> &context)
    : context_(context),
      value_(defaultValue),
      defaultValue_(defaultValue),
      minValue_(minValue),
      maxValue_(maxValue),
      eventRenderQueue_(defaultValue),
      audioBuffer_(
          std::make_shared<DSPAudioBuffer>(RENDER_QUANTUM_SIZE, 1, context->getSampleRate())) {
  inputBuffers_.reserve(4);
  inputNodes_.reserve(4);
}

float AudioParam::getValueAtTime(double time) {
  auto value = eventRenderQueue_.computeValueAtTime(time);
  if (!value.has_value()) {
    return value_.load(std::memory_order_relaxed);
  }
  setValue(value.value());
  return value.value();
}

void AudioParam::setValueAtTime(float value, double startTime) {
  this->updateQueue(ParamRenderEventFactory::createSetValueEvent(value, startTime));
}

void AudioParam::linearRampToValueAtTime(float value, double endTime) {
  this->updateQueue(ParamRenderEventFactory::createLinearRampEvent(value, endTime));
}

void AudioParam::exponentialRampToValueAtTime(float value, double endTime) {
  this->updateQueue(ParamRenderEventFactory::createExponentialRampEvent(value, endTime));
}

void AudioParam::setTargetAtTime(float target, double startTime, double timeConstant) {
  this->updateQueue(ParamRenderEventFactory::createSetTargetEvent(target, startTime, timeConstant));
}

void AudioParam::setValueCurveAtTime(
    const std::shared_ptr<AudioArray> &values,
    size_t length,
    double startTime,
    double duration) {
  this->updateQueue(
      ParamRenderEventFactory::createSetValueCurveEvent(values, length, startTime, duration));
}

void AudioParam::cancelScheduledValues(double cancelTime) {
  eventRenderQueue_.cancelScheduledValues(cancelTime);
}

void AudioParam::cancelAndHoldAtTime(double cancelTime) {
  eventRenderQueue_.cancelAndHoldAtTime(cancelTime);
}

void AudioParam::addInputNode(AudioNode *node) {
  inputNodes_.emplace_back(node);
}

void AudioParam::removeInputNode(AudioNode *node) {
  for (int i = 0; i < inputNodes_.size(); i++) {
    if (inputNodes_[i] == node) {
      std::swap(inputNodes_[i], inputNodes_.back());
      inputNodes_.resize(inputNodes_.size() - 1);
      break;
    }
  }
}

std::shared_ptr<DSPAudioBuffer> AudioParam::calculateInputs(
    const std::shared_ptr<DSPAudioBuffer> &processingBuffer,
    int framesToProcess) {
  processingBuffer->zero();
  if (inputNodes_.empty()) {
    return processingBuffer;
  }
  processInputs(processingBuffer, framesToProcess, true);
  mixInputsBuffers(processingBuffer);
  return processingBuffer;
}

std::shared_ptr<DSPAudioBuffer> AudioParam::processARateParam(int framesToProcess, double time) {
  auto processingBuffer = calculateInputs(audioBuffer_, framesToProcess);

  std::shared_ptr<BaseAudioContext> context = context_.lock();
  if (context == nullptr) {
    return processingBuffer;
  }
  float sampleRate = context->getSampleRate();
  auto bufferData = processingBuffer->getChannel(0)->span();
  double timeCache = time;
  float timeStep = 1.0f / sampleRate;
  float sample = 0.0f;

  // Add automated parameter value to each sample
  for (int i = 0; i < framesToProcess; i++, timeCache += timeStep) {
    sample = getValueAtTime(timeCache);
    bufferData[i] += sample;
  }
  // processingBuffer is a mono buffer containing per-sample parameter values
  return processingBuffer;
}

float AudioParam::processKRateParam(int framesToProcess, double time) {
  auto processingBuffer = calculateInputs(audioBuffer_, framesToProcess);

  // Return block-rate parameter value plus first sample of input modulation
  return processingBuffer->getChannel(0)->span()[0] + getValueAtTime(time);
}

void AudioParam::processInputs(
    const std::shared_ptr<DSPAudioBuffer> &outputBuffer,
    int framesToProcess,
    bool checkIsAlreadyProcessed) {
  for (auto *inputNode : inputNodes_) {
    assert(inputNode != nullptr);

    if (!inputNode->isEnabled()) {
      continue;
    }

    // Process this input node and store its output buffer
    auto inputBuffer =
        inputNode->processAudio(outputBuffer, framesToProcess, checkIsAlreadyProcessed);
    inputBuffers_.emplace_back(inputBuffer);
  }
}

void AudioParam::mixInputsBuffers(const std::shared_ptr<DSPAudioBuffer> &processingBuffer) {
  assert(processingBuffer != nullptr);

  // Sum all input buffers into the processing buffer
  for (auto &inputBuffer : inputBuffers_) {
    processingBuffer->sum(*inputBuffer, ChannelInterpretation::SPEAKERS);
  }

  // Clear for next processing cycle
  inputBuffers_.clear();
}

} // namespace audioapi
