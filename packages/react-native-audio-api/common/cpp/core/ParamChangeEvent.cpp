#include "ParamChangeEvent.h"

#include <utility>

namespace audioapi {

ParamChangeEvent::ParamChangeEvent(
    double startTime,
    double endTime,
    float startValue,
    float endValue,
    std::function<float(double, double, float, float, double)> calculateValue,
    ParamChangeEventType type)
    : startTime_(startTime),
      endTime_(endTime),
      startValue_(startValue),
      endValue_(endValue),
      calculateValue_(std::move(calculateValue)),
      type_(type) {}

double ParamChangeEvent::getEndTime() const {
  return endTime_;
}

double ParamChangeEvent::getStartTime() const {
  return startTime_;
}

float ParamChangeEvent::getEndValue() const {
  return endValue_;
}

float ParamChangeEvent::getStartValue() const {
  return startValue_;
}

std::function<float(double, double, float, float, double)>
ParamChangeEvent::getCalculateValue() const {
  return calculateValue_;
}

ParamChangeEventType ParamChangeEvent::getType() const {
  return type_;
}

void ParamChangeEvent::setEndTime(double endTime) {
  endTime_ = endTime;
}

void ParamChangeEvent::setStartValue(float startValue) {
  startValue_ = startValue;
}

void ParamChangeEvent::setEndValue(float endValue) {
  endValue_ = endValue;
}

bool ParamChangeEvent::operator<(const ParamChangeEvent &other) const {
  if (startTime_ != other.startTime_)
    return startTime_ < other.startTime_;
  return endTime_ < other.endTime_;
}

} // namespace audioapi
