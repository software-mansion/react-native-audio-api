#pragma once

#include <audioapi/core/types/AutomationEventType.h>

namespace audioapi {

class BaseAutomationEvent {
 public:
  BaseAutomationEvent() = default;

  explicit BaseAutomationEvent(AutomationEventType type, double startTime, double endTime = 0.0)
      : type_(type), startTime_(startTime), endTime_(endTime) {}

  BaseAutomationEvent(const BaseAutomationEvent &) = delete;
  BaseAutomationEvent &operator=(const BaseAutomationEvent &) = delete;

  BaseAutomationEvent(BaseAutomationEvent &&other) noexcept
      : type_(other.type_), startTime_(other.startTime_), endTime_(other.endTime_) {}

  BaseAutomationEvent &operator=(BaseAutomationEvent &&other) noexcept {
    if (this != &other) {
      type_ = other.type_;
      startTime_ = other.startTime_;
      endTime_ = other.endTime_;
    }
    return *this;
  }

  [[nodiscard]] inline double getAutomationEventTime() const noexcept {
    bool isRamp =
        type_ == AutomationEventType::LINEAR_RAMP || type_ == AutomationEventType::EXPONENTIAL_RAMP;
    return isRamp ? endTime_ : startTime_;
  }

  [[nodiscard]] inline double getStartTime() const noexcept {
    return startTime_;
  }

  [[nodiscard]] inline double getEndTime() const noexcept {
    return endTime_;
  }

  [[nodiscard]] inline AutomationEventType getType() const noexcept {
    return type_;
  }

  inline void setEndTime(double endTime) noexcept {
    endTime_ = endTime;
  }

 protected:
  double startTime_ = 0.0;
  double endTime_ = 0.0;
  AutomationEventType type_;
};

} // namespace audioapi
