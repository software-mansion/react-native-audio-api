#include <audioapi/core/AudioParam.h>
#include <audioapi/core/BaseAudioContext.h>
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
      eventsQueue_(),
      startTime_(0),
      endTime_(0),
      startValue_(defaultValue),
      endValue_(defaultValue),
      inputBuffer_(
          std::make_shared<DSPAudioBuffer>(RENDER_QUANTUM_SIZE, 1, context->getSampleRate())),
      outputBuffer_(
          std::make_shared<DSPAudioBuffer>(RENDER_QUANTUM_SIZE, 1, context->getSampleRate())) {
  // Default calculation function just returns the static value
  calculateValue_ = [this](double, double, float, float, double) {
    return value_.load(std::memory_order_relaxed);
  };
}

float AudioParam::getValueAtTime(double time) {
  // Check if current automation segment has ended and we need to advance to
  // next event
  if (endTime_ < time && !eventsQueue_.isEmpty()) {
    ParamChangeEvent event;
    eventsQueue_.popFront(event);
    startTime_ = event.getStartTime();
    endTime_ = event.getEndTime();
    startValue_ = event.getStartValue();
    endValue_ = event.getEndValue();
    calculateValue_ = event.getCalculateValue();
  }

  // Calculate value using the current automation function and clamp to valid
  auto value = calculateValue_(startTime_, endTime_, startValue_, endValue_, time);
  setValue(value);
  return value;
}

void AudioParam::setValueAtTime(float value, double startTime) {
  // Ignore events scheduled before the end of existing automation
  if (startTime < this->getQueueEndTime()) {
    return;
  }

  // Step function: instant change at startTime
  auto calculateValue =
      [](double startTime, double /* endTime */, float startValue, float endValue, double time) {
        if (time < startTime) {
          return startValue;
        }

        return endValue;
      };

  this->updateQueue(ParamChangeEvent(
      startTime,
      startTime,
      this->getQueueEndValue(),
      value,
      std::move(calculateValue),
      ParamChangeEventType::SET_VALUE));
}

void AudioParam::linearRampToValueAtTime(float value, double endTime) {
  // Ignore events scheduled before the end of existing automation
  if (endTime < this->getQueueEndTime()) {
    return;
  }

  // Linear interpolation function
  auto calculateValue =
      [](double startTime, double endTime, float startValue, float endValue, double time) {
        if (time < startTime) {
          return startValue;
        }

        if (time < endTime) {
          return static_cast<float>(
              startValue + (endValue - startValue) * (time - startTime) / (endTime - startTime));
        }

        return endValue;
      };

  this->updateQueue(ParamChangeEvent(
      this->getQueueEndTime(),
      endTime,
      this->getQueueEndValue(),
      value,
      std::move(calculateValue),
      ParamChangeEventType::LINEAR_RAMP));
}

void AudioParam::exponentialRampToValueAtTime(float value, double endTime) {
  if (endTime <= this->getQueueEndTime()) {
    return;
  }

  // Exponential curve function using power law
  auto calculateValue =
      [](double startTime, double endTime, float startValue, float endValue, double time) {
        if (startValue * endValue < 0 || startValue == 0) {
          return startValue;
        }

        if (time < startTime) {
          return startValue;
        }

        if (time < endTime) {
          return static_cast<float>(
              startValue * pow(endValue / startValue, (time - startTime) / (endTime - startTime)));
        }

        return endValue;
      };

  this->updateQueue(ParamChangeEvent(
      this->getQueueEndTime(),
      endTime,
      this->getQueueEndValue(),
      value,
      std::move(calculateValue),
      ParamChangeEventType::EXPONENTIAL_RAMP));
}

void AudioParam::setTargetAtTime(float target, double startTime, double timeConstant) {
  if (startTime <= this->getQueueEndTime()) {
    return;
  }
  // Exponential decay function towards target value
  auto calculateValue = [timeConstant, target](
                            double startTime, double, float startValue, float, double time) {
    if (timeConstant == 0) {
      return target;
    }

    if (time < startTime) {
      return startValue;
    }

    return static_cast<float>(
        target + (startValue - target) * exp(-(time - startTime) / timeConstant));
  };
  this->updateQueue(ParamChangeEvent(
      startTime,
      startTime, // SetTarget events have infinite duration conceptually
      this->getQueueEndValue(),
      this->getQueueEndValue(), // End value is not meaningful for
                                // infinite events
      std::move(calculateValue),
      ParamChangeEventType::SET_TARGET));
}

void AudioParam::setValueCurveAtTime(
    const std::shared_ptr<AudioArray> &values,
    size_t length,
    double startTime,
    double duration) {
  if (startTime <= this->getQueueEndTime()) {
    return;
  }

  auto calculateValue =
      [values, length](
          double startTime, double endTime, float startValue, float endValue, double time) {
        if (time < startTime) {
          return startValue;
        }

        if (time < endTime) {
          // Calculate position in the array based on time progress
          auto k = static_cast<int>(std::floor(
              static_cast<double>(length - 1) / (endTime - startTime) * (time - startTime)));
          // Calculate interpolation factor between adjacent array elements
          auto factor = static_cast<float>(
              (time - startTime) * static_cast<double>(length - 1) / (endTime - startTime) - k);
          return dsp::linearInterpolate(values->span(), k, k + 1, factor);
        }

        return endValue;
      };

  this->updateQueue(ParamChangeEvent(
      startTime,
      startTime + duration,
      this->getQueueEndValue(),
      values->span()[length - 1],
      std::move(calculateValue),
      ParamChangeEventType::SET_VALUE_CURVE));
}

void AudioParam::cancelScheduledValues(double cancelTime) {
  this->eventsQueue_.cancelScheduledValues(cancelTime);
}

void AudioParam::cancelAndHoldAtTime(double cancelTime) {
  this->eventsQueue_.cancelAndHoldAtTime(cancelTime, this->endTime_);
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
