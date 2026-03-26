#pragma once

#include <audioapi/core/utils/BaseAutomationEvent.hpp>

#include <functional>
#include <utility>
#include "audioapi/core/types/AutomationEventType.h"

namespace audioapi {

class RenderAutomationEvent : public BaseAutomationEvent {
 public:
  RenderAutomationEvent() = default;

  explicit RenderAutomationEvent(
      double startTime,
      double endTime,
      float startValue,
      float endValue,
      std::function<float(double, double, float, float, double)> &&calculateValue,
      AutomationEventType type)
      : BaseAutomationEvent(type, startTime, endTime),
        calculateValue_(std::move(calculateValue)),
        startValue_(startValue),
        endValue_(endValue) {}

  RenderAutomationEvent(const RenderAutomationEvent &) = delete;
  RenderAutomationEvent &operator=(const RenderAutomationEvent &) = delete;

  RenderAutomationEvent(RenderAutomationEvent &&other) noexcept
      : BaseAutomationEvent(std::move(other)),
        calculateValue_(std::move(other.calculateValue_)),
        startValue_(other.startValue_),
        endValue_(other.endValue_) {}

  RenderAutomationEvent &operator=(RenderAutomationEvent &&other) noexcept {
    if (this != &other) {
      BaseAutomationEvent::operator=(std::move(other));
      calculateValue_ = std::move(other.calculateValue_);
      startValue_ = other.startValue_;
      endValue_ = other.endValue_;
    }
    return *this;
  }

  [[nodiscard]] inline float getEndValue() const noexcept {
    return endValue_;
  }

  [[nodiscard]] inline float getStartValue() const noexcept {
    return startValue_;
  }

  [[nodiscard]] inline const std::function<float(double, double, float, float, double)> &
  getCalculateValue() const noexcept {
    return calculateValue_;
  }

  inline void setStartValue(float startValue) noexcept {
    startValue_ = startValue;
  }

  inline void setEndValue(float endValue) noexcept {
    endValue_ = endValue;
  }

 private:
  std::function<float(double, double, float, float, double)> calculateValue_;
  float startValue_ = 0.0f;
  float endValue_ = 0.0f;
};

} // namespace audioapi
