#include <audioapi/core/AudioParam.h>
#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/utils/Locker.h>
#include <audioapi/dsp/AudioUtils.h>
#include <audioapi/dsp/VectorMath.h>
#include <audioapi/utils/AudioArray.h>
#include <iostream>

namespace audioapi {

AudioParam::AudioParam(
    float defaultValue,
    float minValue,
    float maxValue,
    BaseAudioContext *context)
    : value_(defaultValue),
      defaultValue_(defaultValue),
      minValue_(minValue),
      maxValue_(maxValue),
      context_(context),
      audioBus_(
          std::make_shared<AudioBus>(
              RENDER_QUANTUM_SIZE,
              1,
              context->getSampleRate())) {
  startTime_ = 0;
  endTime_ = 0;
  startValue_ = value_;
  endValue_ = value_;
  calculateValue_ = [this](double, double, float, float, double) {
    return value_;
  };
}

float AudioParam::getValue() const {
  return value_;
}

float AudioParam::getDefaultValue() const {
  return defaultValue_;
}

float AudioParam::getMinValue() const {
  return minValue_;
}

float AudioParam::getMaxValue() const {
  return maxValue_;
}

std::mutex &AudioParam::getQueueLock() {
  return queueLock_;
}

void AudioParam::setValue(float value) {
  value_ = std::clamp(value, minValue_, maxValue_);
}

float AudioParam::getValueAtTime(double time) {
  Locker lock(getQueueLock());
  if (endTime_ < time && !eventsQueue_.empty()) {
    auto event = eventsQueue_.front();
    startTime_ = event.getStartTime();
    endTime_ = event.getEndTime();
    startValue_ = event.getStartValue();
    endValue_ = event.getEndValue();
    calculateValue_ = event.getCalculateValue();
    eventsQueue_.pop_front();
  }

  setValue(calculateValue_(startTime_, endTime_, startValue_, endValue_, time));

  return value_;
}

void AudioParam::setValueAtTime(float value, double startTime) {
  if (startTime <= getQueueEndTime()) {
    return;
  }

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

  Locker lock(getQueueLock());
  auto event = ParamChangeEvent(
      startTime,
      startTime,
      getQueueEndValue(),
      value,
      calculateValue,
      ParamChangeEventType::SET_VALUE);
  updateQueue(event);
}

void AudioParam::linearRampToValueAtTime(float value, double endTime) {
  if (endTime <= getQueueEndTime()) {
    return;
  }

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
          (endValue - startValue) * (time - startTime) / (endTime - startTime));
    }

    return endValue;
  };

  Locker lock(getQueueLock());
  auto event = ParamChangeEvent(
      getQueueEndTime(),
      endTime,
      getQueueEndValue(),
      value,
      calculateValue,
      ParamChangeEventType::LINEAR_RAMP);
  updateQueue(event);
}

void AudioParam::exponentialRampToValueAtTime(float value, double endTime) {
  if (endTime <= getQueueEndTime()) {
    return;
  }

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

  Locker lock(getQueueLock());
  auto event = ParamChangeEvent(
      getQueueEndTime(),
      endTime,
      getQueueEndValue(),
      value,
      calculateValue,
      ParamChangeEventType::EXPONENTIAL_RAMP);
  updateQueue(event);
}

void AudioParam::setTargetAtTime(
    float target,
    double startTime,
    double timeConstant) {
  if (startTime <= getQueueEndTime()) {
    return;
  }

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

  Locker lock(getQueueLock());
  auto event = ParamChangeEvent(
      startTime,
      startTime,
      getQueueEndValue(),
      getQueueEndValue(),
      calculateValue,
      ParamChangeEventType::SET_TARGET);
  updateQueue(event);
}

void AudioParam::setValueCurveAtTime(
    const float *values,
    size_t length,
    double startTime,
    double duration) {
  if (startTime <= getQueueEndTime()) {
    return;
  }

  auto calculateValue = [&values, length](
                            double startTime,
                            double endTime,
                            float startValue,
                            float endValue,
                            double time) {
    if (time < startTime) {
      return startValue;
    }

    if (time < endTime) {
      auto k = static_cast<int>(std::floor(
          static_cast<double>(length - 1) / (endTime - startTime) *
          (time - startTime)));
      auto factor = static_cast<float>(
          k -
          (time - startTime) * static_cast<double>(length - 1) /
              (endTime - startTime));

      return dsp::linearInterpolate(values, k, k + 1, factor);
    }

    return endValue;
  };

  Locker lock(getQueueLock());
  auto event = ParamChangeEvent(
      startTime,
      startTime + duration,
      getQueueEndValue(),
      values[length - 1],
      calculateValue,
      ParamChangeEventType::SET_VALUE_CURVE);
  updateQueue(event);
}

void AudioParam::cancelScheduledValues(double cancelTime) {
  Locker lock(getQueueLock());
  auto it = eventsQueue_.rbegin();
  while (it->getEndTime() >= cancelTime) {
    if (it->getStartTime() >= cancelTime ||
        it->getType() == ParamChangeEventType::SET_VALUE_CURVE) {
      eventsQueue_.pop_back();
    }

    it++;
  }
}

void AudioParam::cancelAndHoldAtTime(double cancelTime) {
  Locker lock(getQueueLock());
  auto it = eventsQueue_.rbegin();
  while (it->getEndTime() >= cancelTime) {
    if (it->getStartTime() >= cancelTime) {
      eventsQueue_.pop_back();
    }

    it++;
  }

  if (eventsQueue_.empty()) {
    endTime_ = cancelTime;
  }

  if (!eventsQueue_.empty()) {
    auto lastEvent = eventsQueue_.rbegin();
    if (lastEvent->getEndTime() > cancelTime) {
      lastEvent->setEndTime(cancelTime);
    }
  }
}

void AudioParam::addInputNode(AudioNode *node) {
  auto position = inputNodes_.find(node);
  if (position == inputNodes_.end()) {
    inputNodes_.insert(node);
  }
}

void AudioParam::removeInputNode(AudioNode *node) {
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

std::shared_ptr<AudioBus> AudioParam::processARateParam(
    int framesToProcess,
    double time) {
  auto processingBus = calculateInputs(audioBus_, framesToProcess);
  for (size_t i = 0; i < framesToProcess; i++) {
    auto sample = getValueAtTime(time + i / context_->getSampleRate());
    processingBus->getChannel(0)->getData()[i] += sample;
  }
  // processingBus is a mono bus
  return processingBus;
}

float AudioParam::processKRateParam(int framesToProcess, double time) {
  auto processingBus = calculateInputs(audioBus_, framesToProcess);
  return processingBus->getChannel(0)->getData()[0] + getValueAtTime(time);
}

double AudioParam::getQueueEndTime() {
  if (eventsQueue_.empty()) {
    return endTime_;
  }

  return eventsQueue_.back().getEndTime();
}

float AudioParam::getQueueEndValue() {
  if (eventsQueue_.empty()) {
    return this->endValue_;
  }

  return eventsQueue_.back().getEndValue();
}

void AudioParam::updateQueue(ParamChangeEvent &event) {
  if (!eventsQueue_.empty()) {
    auto prev = eventsQueue_.back();

    if (prev.getType() == ParamChangeEventType::SET_TARGET) {
      prev.setEndTime(event.getStartTime());
      prev.setEndValue(prev.getCalculateValue()(
          prev.getStartTime(),
          prev.getEndTime(),
          prev.getStartValue(),
          prev.getEndValue(),
          event.getStartTime()));
    }

    event.setStartValue(prev.getEndValue());
  }

  eventsQueue_.push_back(event);
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

    auto inputBus = inputNode->processAudio(
        outputBus, framesToProcess, checkIsAlreadyProcessed);
    inputBuses_.push_back(inputBus);
  }
}

void AudioParam::mixInputsBuses(
    const std::shared_ptr<AudioBus> &processingBus) {
  assert(processingBus != nullptr);

  for (auto it = inputBuses_.begin(), end = inputBuses_.end(); it != end;
       ++it) {
    processingBus->sum(it->get(), ChannelInterpretation::SPEAKERS);
  }

  inputBuses_.clear();
}

} // namespace audioapi
