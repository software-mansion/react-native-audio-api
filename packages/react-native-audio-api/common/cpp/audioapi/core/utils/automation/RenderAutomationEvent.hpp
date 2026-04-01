#pragma once

#include <audioapi/core/utils/automation/AutomationEvent.hpp>

#include <functional>
#include <utility>
#include "audioapi/core/types/AutomationEventType.h"

namespace audioapi {

class RenderAutomationEvent : public AutomationEvent {
 public:
  RenderAutomationEvent() = default;
  ~RenderAutomationEvent() = default;

  explicit RenderAutomationEvent(
      double startTime,
      double endTime,
      float startValue,
      float endValue,
      std::function<float(double, double, float, float, double)> &&calculateValue,
      AutomationEventType type)
      : AutomationEvent(type, startTime, endTime),
        calculateValue_(std::move(calculateValue)),
        startValue_(startValue),
        endValue_(endValue) {}

  RenderAutomationEvent(const RenderAutomationEvent &) = delete;
  RenderAutomationEvent &operator=(const RenderAutomationEvent &) = delete;

  RenderAutomationEvent(RenderAutomationEvent &&other) noexcept
      : AutomationEvent(std::move(other)),
        calculateValue_(std::move(other.calculateValue_)),
        startValue_(other.startValue_),
        endValue_(other.endValue_) {}

  RenderAutomationEvent &operator=(RenderAutomationEvent &&other) noexcept {
    if (this != &other) {
      AutomationEvent::operator=(std::move(other));
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
