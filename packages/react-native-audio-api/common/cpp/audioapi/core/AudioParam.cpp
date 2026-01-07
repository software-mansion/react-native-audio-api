#include <audioapi/core/AudioParam.h>
#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/dsp/AudioUtils.h>
#include <audioapi/dsp/VectorMath.h>
#include <audioapi/utils/AudioArray.h>
#include <memory>
#include <utility>

namespace audioapi {

AudioParam::AudioParam(
    float defaultValue,
    float minValue,
    float maxValue,
    std::shared_ptr<BaseAudioContext> context)
    : context_(context),
      value_(defaultValue),
      defaultValue_(defaultValue),
      minValue_(minValue),
      maxValue_(maxValue),
      eventsQueue_(),
      eventScheduler_(32),
      startTime_(0),
      endTime_(0),
      startValue_(defaultValue),
      endValue_(defaultValue),
      audioBus_(std::make_shared<AudioBus>(RENDER_QUANTUM_SIZE, 1, context->getSampleRate())) {
  inputBuses_.reserve(4);
  inputNodes_.reserve(4);
  // Default calculation function just returns the static value
  calculateValue_ = [this](double, double, float, float, double) {
    return value_;
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
    calculateValue_ = std::move(event.getCalculateValue());
  }

  // Calculate value using the current automation function and clamp to valid
  setValue(calculateValue_(startTime_, endTime_, startValue_, endValue_, time));
  return value_;
}

void AudioParam::setValueAtTime(float value, double startTime) {
  auto event = [value, startTime](AudioParam &param) {
    // Ignore events scheduled before the end of existing automation
    if (startTime < param.getQueueEndTime()) {
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

    param.updateQueue(ParamChangeEvent(
        startTime,
        startTime,
        param.getQueueEndValue(),
        value,
        std::move(calculateValue),
        ParamChangeEventType::SET_VALUE));
  };
  eventScheduler_.scheduleEvent(std::move(event));
}

void AudioParam::linearRampToValueAtTime(float value, double endTime) {
  auto event = [value, endTime](AudioParam &param) {
    // Ignore events scheduled before the end of existing automation
    if (endTime < param.getQueueEndTime()) {
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

    param.updateQueue(ParamChangeEvent(
        param.getQueueEndTime(),
        endTime,
        param.getQueueEndValue(),
        value,
        std::move(calculateValue),
        ParamChangeEventType::LINEAR_RAMP));
  };
  eventScheduler_.scheduleEvent(std::move(event));
}

void AudioParam::exponentialRampToValueAtTime(float value, double endTime) {
  auto event = [value, endTime](AudioParam &param) {
    if (endTime <= param.getQueueEndTime()) {
      return;
    }

    // Exponential curve function using power law
    auto calculateValue =
        [](double startTime, double endTime, float startValue, float endValue, double time) {
          if (time < startTime) {
            return startValue;
          }

          if (time < endTime) {
            return static_cast<float>(
                startValue *
                pow(endValue / startValue, (time - startTime) / (endTime - startTime)));
          }

          return endValue;
        };

    param.updateQueue(ParamChangeEvent(
        param.getQueueEndTime(),
        endTime,
        param.getQueueEndValue(),
        value,
        std::move(calculateValue),
        ParamChangeEventType::EXPONENTIAL_RAMP));
  };
  eventScheduler_.scheduleEvent(std::move(event));
}

void AudioParam::setTargetAtTime(float target, double startTime, double timeConstant) {
  auto event = [target, startTime, timeConstant](AudioParam &param) {
    if (startTime <= param.getQueueEndTime()) {
      return;
    }
    // Exponential decay function towards target value
    auto calculateValue = [timeConstant, target](
                              double startTime, double, float startValue, float, double time) {
      if (time < startTime) {
        return startValue;
      }

      return static_cast<float>(
          target + (startValue - target) * exp(-(time - startTime) / timeConstant));
    };
    param.updateQueue(ParamChangeEvent(
        startTime,
        startTime, // SetTarget events have infinite duration conceptually
        param.getQueueEndValue(),
        param.getQueueEndValue(), // End value is not meaningful for
                                  // infinite events
        std::move(calculateValue),
        ParamChangeEventType::SET_TARGET));
  };

  eventScheduler_.scheduleEvent(std::move(event));
}

void AudioParam::setValueCurveAtTime(
    std::shared_ptr<std::vector<float>> values,
    size_t length,
    double startTime,
    double duration) {
  auto event = [values, length, startTime, duration](AudioParam &param) {
    if (startTime <= param.getQueueEndTime()) {
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
            return dsp::linearInterpolate(values->data(), k, k + 1, factor);
          }

          return endValue;
        };

    param.updateQueue(ParamChangeEvent(
        startTime,
        startTime + duration,
        param.getQueueEndValue(),
        values->at(length - 1),
        std::move(calculateValue),
        ParamChangeEventType::SET_VALUE_CURVE));
  };

  /// Schedules an event that modifies this param
  /// It will be executed on next audio render cycle
  eventScheduler_.scheduleEvent(std::move(event));
}

void AudioParam::cancelScheduledValues(double cancelTime) {
  eventScheduler_.scheduleEvent(
      [cancelTime](AudioParam &param) { param.eventsQueue_.cancelScheduledValues(cancelTime); });
}

void AudioParam::cancelAndHoldAtTime(double cancelTime) {
  eventScheduler_.scheduleEvent([cancelTime](AudioParam &param) {
    param.eventsQueue_.cancelAndHoldAtTime(cancelTime, param.endTime_);
  });
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

std::shared_ptr<AudioBus> AudioParam::calculateInputs(
    const std::shared_ptr<AudioBus> &processingBus,
    int framesToProcess) {
  processingBus->zero();
  if (inputNodes_.empty()) {
    return processingBus;
  }
  processInputs(processingBus, framesToProcess, true);
  mixInputsBuses(processingBus);
  return processingBus;
}

std::shared_ptr<AudioBus> AudioParam::processARateParam(int framesToProcess, double time) {
  processScheduledEvents();
  auto processingBus = calculateInputs(audioBus_, framesToProcess);

  std::shared_ptr<BaseAudioContext> context = context_.lock();
  if (context == nullptr)
    return processingBus;
  float sampleRate = context->getSampleRate();
  float *busData = processingBus->getChannel(0)->getData();
  float timeCache = time;
  float timeStep = 1.0f / sampleRate;
  float sample = 0.0f;

  // Add automated parameter value to each sample
  for (size_t i = 0; i < framesToProcess; i++, timeCache += timeStep) {
    sample = getValueAtTime(timeCache);
    busData[i] += sample;
  }
  // processingBus is a mono bus containing per-sample parameter values
  return processingBus;
}

float AudioParam::processKRateParam(int framesToProcess, double time) {
  processScheduledEvents();
  auto processingBus = calculateInputs(audioBus_, framesToProcess);

  // Return block-rate parameter value plus first sample of input modulation
  return processingBus->getChannel(0)->getData()[0] + getValueAtTime(time);
}

void AudioParam::processInputs(
    const std::shared_ptr<AudioBus> &outputBus,
    int framesToProcess,
    bool checkIsAlreadyProcessed) {
  for (auto it = inputNodes_.begin(), end = inputNodes_.end(); it != end; ++it) {
    auto inputNode = *it;
    assert(inputNode != nullptr);

    if (!inputNode->isEnabled()) {
      continue;
    }

    // Process this input node and store its output bus
    auto inputBus = inputNode->processAudio(outputBus, framesToProcess, checkIsAlreadyProcessed);
    inputBuses_.emplace_back(inputBus);
  }
}

void AudioParam::mixInputsBuses(const std::shared_ptr<AudioBus> &processingBus) {
  assert(processingBus != nullptr);

  // Sum all input buses into the processing bus
  for (auto it = inputBuses_.begin(), end = inputBuses_.end(); it != end; ++it) {
    processingBus->sum(it->get(), ChannelInterpretation::SPEAKERS);
  }

  // Clear for next processing cycle
  inputBuses_.clear();
}

} // namespace audioapi
