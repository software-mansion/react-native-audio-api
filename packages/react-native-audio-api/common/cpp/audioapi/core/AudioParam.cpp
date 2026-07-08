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
      inputBuffer_(
          std::make_shared<DSPAudioBuffer>(RENDER_QUANTUM_SIZE, 1, context->getSampleRate())),
      outputBuffer_(
          std::make_shared<DSPAudioBuffer>(RENDER_QUANTUM_SIZE, 1, context->getSampleRate())),
      eventRenderQueue_(defaultValue) {}

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

std::shared_ptr<DSPAudioBuffer> AudioParam::processARateParam(int framesToProcess, double time) {
  std::shared_ptr<BaseAudioContext> context = context_.lock();
  if (context == nullptr) {
    outputBuffer_->zero();
    return outputBuffer_;
  }

  float sampleRate = context->getSampleRate();
  double timeCache = time;
  double timeStep = 1.0 / sampleRate;

  // Read modulation from input buffer (filled by BridgeNode if connected, otherwise zeros)
  auto inputData = inputBuffer_->getChannel(0)->span();
  auto outputData = outputBuffer_->getChannel(0)->span();

  // Compute: modulation + automated parameter value → output buffer
  for (size_t i = 0; i < framesToProcess; i++, timeCache += timeStep) {
    outputData[i] = inputData[i] + getValueAtTime(timeCache);
  }

  // Zero the input buffer so next frame starts clean if no BridgeNode refills it
  inputBuffer_->zero();

  return outputBuffer_;
}

float AudioParam::processKRateParam(int framesToProcess, double time) {
  // Return block-rate parameter value plus first sample of input modulation
  float modulation = inputBuffer_->getChannel(0)->span()[0];
  float result = modulation + getValueAtTime(time);

  // Zero the input buffer so next frame starts clean if no BridgeNode refills it
  inputBuffer_->zero();

  return result;
}

} // namespace audioapi
