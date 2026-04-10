#pragma once

#include <audioapi/core/types/ParamEventType.h>

namespace audioapi {

/// @brief Base class for param events, containing common properties like startTime, endTime, and type.
class ParamEvent {
 public:
  ParamEvent() = default;
  ~ParamEvent() = default;

  /// @brief Construct an event with explicit start and end times.
  explicit ParamEvent(ParamEventType type, double startTime, double endTime)
      : type_(type), startTime_(startTime), endTime_(endTime) {}

  /// @brief Construct from a single automationTime value, setting startTime or endTime based on type.
  /// Ramp events (LINEAR_RAMP, EXPONENTIAL_RAMP) store automationTime as endTime.
  /// All other types store it as startTime.
  explicit ParamEvent(ParamEventType type, double automationTime)
      : type_(type),
        startTime_(isRamp(type) ? 0.0 : automationTime),
        endTime_(isRamp(type) ? automationTime : 0.0) {}

  ParamEvent(const ParamEvent &) = default;
  ParamEvent &operator=(const ParamEvent &) = default;

  ParamEvent(ParamEvent &&other) noexcept
      : type_(other.type_), startTime_(other.startTime_), endTime_(other.endTime_) {}

  ParamEvent &operator=(ParamEvent &&other) noexcept {
    if (this != &other) {
      type_ = other.type_;
      startTime_ = other.startTime_;
      endTime_ = other.endTime_;
    }
    return *this;
  }

  /// @brief Get the automation time of the event. For ramp events, this is the end time; for other events, it's the start time.
  /// @return The automation time of the event.
  [[nodiscard]] double getAutomationTime() const noexcept {
    return isRamp(type_) ? endTime_ : startTime_;
  }

  [[nodiscard]] double getStartTime() const noexcept {
    return startTime_;
  }

  [[nodiscard]] double getEndTime() const noexcept {
    return endTime_;
  }

  [[nodiscard]] ParamEventType getType() const noexcept {
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
  ParamEventType type_ = ParamEventType::SET_VALUE;

 private:
  static bool isRamp(ParamEventType type) noexcept {
    return type == ParamEventType::LINEAR_RAMP || type == ParamEventType::EXPONENTIAL_RAMP;
  }
};

} // namespace audioapi
