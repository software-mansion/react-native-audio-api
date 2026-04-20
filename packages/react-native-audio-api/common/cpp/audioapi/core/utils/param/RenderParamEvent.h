#pragma once

#include <audioapi/core/utils/param/ParamEvent.h>

#include <audioapi/core/types/ParamEventType.h>
#include <functional>
#include <utility>

namespace audioapi {

/// @brief A RenderParamEvent extends ParamEvent with additional properties and a value calculation
/// function that can compute the parameter value at any time during the event's active period
/// based on its type and the current state of the queue.
class RenderParamEvent : public ParamEvent {
 public:
  RenderParamEvent() = default;
  ~RenderParamEvent() = default;

  explicit RenderParamEvent(
      double startTime,
      double endTime,
      float startValue,
      float endValue,
      std::function<float(double, double, float, float, double)> &&calculateValue,
      ParamEventType type)
      : ParamEvent(type, startTime, endTime),
        calculateValue_(std::move(calculateValue)),
        startValue_(startValue),
        endValue_(endValue) {}

  RenderParamEvent(const RenderParamEvent &) = delete;
  RenderParamEvent &operator=(const RenderParamEvent &) = delete;

  RenderParamEvent(RenderParamEvent &&other) noexcept
      : ParamEvent(std::move(other)),
        calculateValue_(std::move(other.calculateValue_)),
        startValue_(other.startValue_),
        endValue_(other.endValue_) {}

  RenderParamEvent &operator=(RenderParamEvent &&other) noexcept {
    if (this != &other) {
      ParamEvent::operator=(std::move(other));
      calculateValue_ = std::move(other.calculateValue_);
      startValue_ = other.startValue_;
      endValue_ = other.endValue_;
    }
    return *this;
  }

  [[nodiscard]] float getEndValue() const noexcept {
    return endValue_;
  }

  [[nodiscard]] float getStartValue() const noexcept {
    return startValue_;
  }

  [[nodiscard]] const std::function<float(double, double, float, float, double)> &
  getCalculateValue() const noexcept {
    return calculateValue_;
  }

  void setStartValue(float startValue) noexcept {
    startValue_ = startValue;
  }

  void setEndValue(float endValue) noexcept {
    endValue_ = endValue;
  }

 private:
  std::function<float(double, double, float, float, double)> calculateValue_;
  float startValue_ = 0.0f;
  float endValue_ = 0.0f;
};

} // namespace audioapi
