#include <audioapi/core/AudioParam.h>
#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/utils/param/ParamRenderEventFactory.h>
#include <audioapi/dsp/AudioUtils.h>
#include <audioapi/dsp/VectorMath.h>
#include <audioapi/utils/AudioArray.hpp>

#include <cmath>
#include <memory>

namespace audioapi {

AudioParam::AudioParam(
    float defaultValue,
    float minValue,
    float maxValue,
    const std::shared_ptr<BaseAudioContext> &context)
    : GeneralizedAudioParam(minValue, maxValue, context),
      value_(defaultValue),
      defaultValue_(defaultValue),
      eventRenderQueue_(defaultValue),
      inputBuffer_(
          std::make_shared<DSPAudioBuffer>(RENDER_QUANTUM_SIZE, 1, context->getSampleRate())) {}

float AudioParam::getValueAtTimeUnmodulated(double time) {
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
  if (aRateCacheHit(framesToProcess, time)) {
    return outputBuffer_;
  }

  std::shared_ptr<BaseAudioContext> context = context_.lock();
  if (context == nullptr) {
    outputBuffer_->zero();
    return outputBuffer_;
  }

  const double sampleRate = static_cast<double>(context->getSampleRate());
  const auto startFrame = static_cast<std::size_t>(std::llround(time * sampleRate));

  // Read modulation from input buffer (filled by BridgeNode if connected, otherwise zeros)
  auto inputData = inputBuffer_->getChannel(0)->span();
  auto outputData = outputBuffer_->getChannel(0)->span();

  for (int i = 0; i < framesToProcess; ++i) {
    const double sampleTime =
        static_cast<double>(startFrame + static_cast<std::size_t>(i)) / sampleRate;
    outputData[i] = inputData[i] + getValueAtTimeUnmodulated(sampleTime);
  }

  inputBuffer_->zero();

  finalizeARate(framesToProcess, time);
  return outputBuffer_;
}

float AudioParam::processKRateParam(double time) {
  if (kRateCacheHit(time)) {
    return cachedKRateValue_;
  }

  // Return block-rate parameter value plus first sample of input modulation
  float modulation = inputBuffer_->getChannel(0)->span()[0];
  float raw = modulation + getValueAtTimeUnmodulated(time);

  inputBuffer_->zero();

  return finalizeKRate(raw, time);
}

} // namespace audioapi
