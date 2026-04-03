#pragma once

#include <audioapi/core/types/AutomationEventType.h>

namespace audioapi {

class AutomationEvent {
 public:
  AutomationEvent() = default;
  ~AutomationEvent() = default;

  explicit AutomationEvent(AutomationEventType type, double startTime, double endTime)
      : type_(type), startTime_(startTime), endTime_(endTime) {}

  /// @brief Construct from a single automationTime value, setting startTime or endTime based on type.
  /// Ramp events (LINEAR_RAMP, EXPONENTIAL_RAMP) store automationTime as endTime.
  /// All other types store it as startTime.
  explicit AutomationEvent(AutomationEventType type, double automationTime)
      : type_(type),
        startTime_(isRamp(type) ? 0.0 : automationTime),
        endTime_(isRamp(type) ? automationTime : 0.0) {}

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

  [[nodiscard]] double getAutomationTime() const noexcept {
    return isRamp(type_) ? endTime_ : startTime_;
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

  [[nodiscard]] bool isRampType() const noexcept {
    return isRamp(type_);
  }

  void setEndTime(double endTime) noexcept {
    endTime_ = endTime;
  }

  void setStartTime(double startTime) noexcept {
    startTime_ = startTime;
  }

 protected:
  double startTime_ = 0.0;
  double endTime_ = 0.0;
  AutomationEventType type_ = AutomationEventType::SET_VALUE;

 private:
  static bool isRamp(AutomationEventType type) noexcept {
    return type == AutomationEventType::LINEAR_RAMP ||
        type == AutomationEventType::EXPONENTIAL_RAMP;
  }
};

} // namespace audioapi
