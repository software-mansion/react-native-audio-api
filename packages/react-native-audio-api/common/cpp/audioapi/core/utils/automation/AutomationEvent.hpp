#pragma once

#include <audioapi/core/types/AutomationEventType.h>

namespace audioapi {

class AutomationEvent {
 public:
  AutomationEvent() = default;
  ~AutomationEvent() = default;

  explicit AutomationEvent(AutomationEventType type, double startTime, double endTime = 0.0)
      : type_(type), startTime_(startTime), endTime_(endTime) {}

  AutomationEvent(const AutomationEvent &) = delete;
  AutomationEvent &operator=(const AutomationEvent &) = delete;

  AutomationEvent(AutomationEvent &&other) noexcept
      : type_(other.type_), startTime_(other.startTime_), endTime_(other.endTime_) {}

  AutomationEvent &operator=(AutomationEvent &&other) noexcept {
    if (this != &other) {
      type_ = other.type_;
      startTime_ = other.startTime_;
      endTime_ = other.endTime_;
    }
    return *this;
  }

  [[nodiscard]] double getAutomationEventTime() const noexcept {
    bool isRamp =
        type_ == AutomationEventType::LINEAR_RAMP || type_ == AutomationEventType::EXPONENTIAL_RAMP;
    return isRamp ? endTime_ : startTime_;
  }

  [[nodiscard]] double getStartTime() const noexcept {
    return startTime_;
  }

  [[nodiscard]] double getEndTime() const noexcept {
    return endTime_;
  }

  [[nodiscard]] AutomationEventType getType() const noexcept {
    return type_;
  }

  void setEndTime(double endTime) noexcept {
    endTime_ = endTime;
  }

 protected:
  double startTime_ = 0.0;
  double endTime_ = 0.0;
  AutomationEventType type_ = AutomationEventType::SET_VALUE;
};

} // namespace audioapi
