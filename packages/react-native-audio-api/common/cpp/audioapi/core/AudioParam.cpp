#include <audioapi/core/AudioParam.h>
#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/dsp/AudioUtils.h>
#include <audioapi/dsp/VectorMath.h>
#include <audioapi/utils/AudioArray.h>

namespace audioapi {

/**
 * Constructor initializes the parameter with constraints and sets up the
 * working audio bus. The calculateValue_ function is initialized to simply
 * return the current value (no automation).
 */
AudioParam::AudioParam(
    float defaultValue,
    float minValue,
    float maxValue,
    BaseAudioContext *context)
    : context_(context),
      value_(defaultValue),
      defaultValue_(defaultValue),
      minValue_(minValue),
      maxValue_(maxValue),
      eventsQueue_(),
      eventScheduler_(32),
      audioBus_(
          std::make_shared<AudioBus>(
              RENDER_QUANTUM_SIZE,
              1,
              context->getSampleRate())) {
  startTime_ = 0;
  endTime_ = 0;
  startValue_ = value_;
  endValue_ = value_;
  // Default calculation function just returns the static value
  calculateValue_ = [this](double, double, float, float, double) {
    return value_;
  };
}

/**
 * Core automation processing method. This advances through the event queue when
 * the current automation segment ends, loads the next event's parameters into
 * the cached state variables, and calculates the parameter value for the
 * specified time using the active calculation function.
 */
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

/**
 * Schedules an instant value change. The calculateValue function returns
 * startValue before the scheduled time and endValue at or after the scheduled
 * time (step function).
 */
void AudioParam::setValueAtTime(float value, double startTime) {
  return;
  auto event = [value, startTime](AudioParam &param) {
    // Ignore events scheduled before the end of existing automation
    if (startTime <= param.getQueueEndTime()) {
      return;
    }

    // Step function: instant change at startTime
    auto calculateValue = [](double startTime,
                             double,
                             float startValue,
                             float endValue,
                             double time) {
      if (time < startTime) {
        return startValue;
      }

      return endValue;
    };

    param.updateQueue(
        std::move(ParamChangeEvent(
            startTime,
            startTime,
            param.getQueueEndValue(),
            value,
            std::move(calculateValue),
            ParamChangeEventType::SET_VALUE)));
  };
  eventScheduler_.scheduleEvent(std::move(event), *this);
}

/**
 * Schedules a linear interpolation from the end of previous automation to the
 * target value. The calculateValue function implements linear interpolation:
 * startValue + (endValue - startValue) * progress
 */
void AudioParam::linearRampToValueAtTime(float value, double endTime) {
  auto event = [value, endTime](AudioParam &param) {
    // Ignore events scheduled before the end of existing automation
    if (endTime <= param.getQueueEndTime()) {
      return;
    }

    // Linear interpolation function
    auto calculateValue = [](double startTime,
                             double endTime,
                             float startValue,
                             float endValue,
                             double time) {
      if (time < startTime) {
        return startValue;
      }

      if (time < endTime) {
        return static_cast<float>(
            startValue +
            (endValue - startValue) * (time - startTime) /
                (endTime - startTime));
      }

      return endValue;
    };

    param.updateQueue(
        std::move(ParamChangeEvent(
            param.getQueueEndTime(),
            endTime,
            param.getQueueEndValue(),
            value,
            std::move(calculateValue),
            ParamChangeEventType::LINEAR_RAMP)));
  };
  eventScheduler_.scheduleEvent(std::move(event), *this);
}

/**
 * Schedules an exponential curve from the end of previous automation to the
 * target value. Uses power function: startValue *
 * (endValue/startValue)^progress for smooth exponential curves.
 */
void AudioParam::exponentialRampToValueAtTime(float value, double endTime) {
  auto event = [value, endTime](AudioParam &param) {
    if (endTime <= param.getQueueEndTime()) {
      return;
    }

    // Exponential curve function using power law
    auto calculateValue = [](double startTime,
                             double endTime,
                             float startValue,
                             float endValue,
                             double time) {
      if (time < startTime) {
        return startValue;
      }

      if (time < endTime) {
        return static_cast<float>(
            startValue *
            pow(endValue / startValue,
                (time - startTime) / (endTime - startTime)));
      }

      return endValue;
    };

    param.updateQueue(
        std::move(ParamChangeEvent(
            param.getQueueEndTime(),
            endTime,
            param.getQueueEndValue(),
            value,
            std::move(calculateValue),
            ParamChangeEventType::EXPONENTIAL_RAMP)));
  };
  eventScheduler_.scheduleEvent(std::move(event), *this);
}

/**
 * Schedules an exponential approach to a target value with first-order
 * exponential decay. Uses the formula: target + (startValue - target) *
 * e^(-(time - startTime) / timeConstant) This creates a smooth asymptotic
 * approach that theoretically never reaches the target.
 */
void AudioParam::setTargetAtTime(
    float target,
    double startTime,
    double timeConstant) {
  auto event = [target, startTime, timeConstant](AudioParam &param) {
    if (startTime <= param.getQueueEndTime()) {
      return;
    }
    // Exponential decay function towards target value
    auto calculateValue =
        [timeConstant, target](
            double startTime, double, float startValue, float, double time) {
          if (time < startTime) {
            return startValue;
          }

          return static_cast<float>(
              target +
              (startValue - target) * exp(-(time - startTime) / timeConstant));
        };
    param.updateQueue(
        std::move(ParamChangeEvent(
            startTime,
            startTime, // SetTarget events have infinite duration conceptually
            param.getQueueEndValue(),
            param.getQueueEndValue(), // End value is not meaningful for
                                      // infinite events
            std::move(calculateValue),
            ParamChangeEventType::SET_TARGET)));
  };

  eventScheduler_.scheduleEvent(std::move(event), *this);
}

/**
 * Schedules the parameter to follow a custom curve defined by an array of
 * values. The curve is sampled with linear interpolation between adjacent array
 * elements. Progress through the array is calculated as: (time - startTime) /
 * duration * (length - 1)
 */
void AudioParam::setValueCurveAtTime(
    std::shared_ptr<std::vector<float>> values,
    size_t length,
    double startTime,
    double duration) {
  auto event = [values, length, startTime, duration](AudioParam &param) {
    if (startTime <= param.getQueueEndTime()) {
      return;
    }

    auto calculateValue = [values, length](
                              double startTime,
                              double endTime,
                              float startValue,
                              float endValue,
                              double time) {
      if (time < startTime) {
        return startValue;
      }

      if (time < endTime) {
        // Calculate position in the array based on time progress
        auto k = static_cast<int>(std::floor(
            static_cast<double>(length - 1) / (endTime - startTime) *
            (time - startTime)));
        // Calculate interpolation factor between adjacent array elements
        auto factor = static_cast<float>(
            k -
            (time - startTime) * static_cast<double>(length - 1) /
                (endTime - startTime));
        return dsp::linearInterpolate(values->data(), k, k + 1, factor);
      }

      return endValue;
    };

    param.updateQueue(
        std::move(ParamChangeEvent(
            startTime,
            startTime + duration,
            param.getQueueEndValue(),
            values->at(length - 1),
            std::move(calculateValue),
            ParamChangeEventType::SET_VALUE_CURVE)));
  };

  /// Schedules an event that modifies this param
  /// It will be executed on next audio render cycle
  eventScheduler_.scheduleEvent(std::move(event), *this);
}

void AudioParam::cancelScheduledValues(double cancelTime) {
  eventScheduler_.scheduleEvent(
      [cancelTime](AudioParam &param) {
        param.eventsQueue_.cancelScheduledValues(cancelTime);
      },
      *this);
}

void AudioParam::cancelAndHoldAtTime(double cancelTime) {
  eventScheduler_.scheduleEvent(
      [cancelTime](AudioParam &param) {
        param.eventsQueue_.cancelAndHoldAtTime(cancelTime, param.endTime_);
      },
      *this);
}

void AudioParam::addInputNode(AudioNode *node) {
  /// should also be handled by events scheduling TODO
  auto position = inputNodes_.find(node);
  if (position == inputNodes_.end()) {
    inputNodes_.insert(node);
  }
}

void AudioParam::removeInputNode(AudioNode *node) {
  /// should also be handled by events scheduling TODO
  auto position = inputNodes_.find(node);

  if (position != inputNodes_.end()) {
    inputNodes_.erase(position);
  }
}

std::shared_ptr<AudioBus> AudioParam::calculateInputs(
    const std::shared_ptr<AudioBus> &processingBus,
    int framesToProcess) {
  processingBus->zero();
  if (!inputNodes_.empty()) {
    processInputs(processingBus, framesToProcess, true);
    mixInputsBuses(processingBus);
  }
  return processingBus;
}

/**
 * Processes parameter values at audio rate (per-sample precision).
 * For each audio frame, calculates the automated parameter value and adds any
 * input modulation. Returns an AudioBus where each sample contains the
 * parameter value for that time instant.
 */
std::shared_ptr<AudioBus> AudioParam::processARateParam(
    int framesToProcess,
    double time) {
  processScheduledEvents();
  auto processingBus = calculateInputs(audioBus_, framesToProcess);

  // Add automated parameter value to each sample
  for (size_t i = 0; i < framesToProcess; i++) {
    auto sample = getValueAtTime(time + i / context_->getSampleRate());
    processingBus->getChannel(0)->getData()[i] += sample;
  }
  // processingBus is a mono bus containing per-sample parameter values
  return processingBus;
}

/**
 * Processes parameter values at control rate (per-block precision).
 * Calculates a single parameter value for the entire audio block and adds input
 * modulation. More efficient than A-rate processing but with lower time
 * precision.
 */
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
  for (auto it = inputNodes_.begin(), end = inputNodes_.end(); it != end;
       ++it) {
    auto inputNode = *it;
    assert(inputNode != nullptr);

    if (!inputNode->isEnabled()) {
      continue;
    }

    // Process this input node and store its output bus
    auto inputBus = inputNode->processAudio(
        outputBus, framesToProcess, checkIsAlreadyProcessed);
    inputBuses_.push_back(inputBus);
  }
}

/**
 * Mixes all processed input buses together using sum mixing.
 * After mixing, clears the inputBuses_ vector for the next processing cycle.
 */
void AudioParam::mixInputsBuses(
    const std::shared_ptr<AudioBus> &processingBus) {
  assert(processingBus != nullptr);

  // Sum all input buses into the processing bus
  for (auto it = inputBuses_.begin(), end = inputBuses_.end(); it != end;
       ++it) {
    processingBus->sum(it->get(), ChannelInterpretation::SPEAKERS);
  }

  // Clear for next processing cycle
  inputBuses_.clear();
}

} // namespace audioapi
