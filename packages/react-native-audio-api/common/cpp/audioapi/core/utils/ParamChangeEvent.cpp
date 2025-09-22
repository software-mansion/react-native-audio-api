#include <audioapi/core/utils/ParamChangeEvent.h>

namespace audioapi {

ParamChangeEvent::ParamChangeEvent(
    double startTime,
    double endTime,
    float startValue,
    float endValue,
    std::function<float(double, double, float, float, double)> &&calculateValue,
    ParamChangeEventType type)
    : startTime_(startTime),
      endTime_(endTime),
      calculateValue_(std::move(calculateValue)),
      startValue_(startValue),
      endValue_(endValue),
      type_(type) {}

} // namespace audioapi
