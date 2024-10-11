#include "ParamChange.h"

#include <utility>

namespace audioapi {

ParamChange::ParamChange(
    double startTime,
    double endTime,
    float startValue,
    float endValue,
    std::function<float(double, double, float, float, double)> calculateValue)
    : startTime_(startTime),
      endTime_(endTime),
      startValue_(startValue),
      endValue_(endValue),
      calculateValue_(std::move(calculateValue)) {}

double ParamChange::getEndTime() const {
  return endTime_;
}

double ParamChange::getStartTime() const {
  return startTime_;
}

float ParamChange::getEndValue() const {
  return endValue_;
}

float ParamChange::getStartValue() const {
  return startValue_;
}

std::function<float(double, double, float, float, double)> ParamChange::getCalculateValue() const {
  return calculateValue_;
}

bool ParamChange::operator<(const ParamChange &other) const {
  if (startTime_ != other.startTime_)
    return startTime_ < other.startTime_;
  return endTime_ < other.endTime_;
}

} // namespace audioapi
