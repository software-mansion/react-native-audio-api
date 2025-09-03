#pragma once

#include <audioapi/core/types/ParamChangeEventType.h>

#include <functional>
#include <memory>
#include <utility>

namespace audioapi {

class ParamChangeEvent {
 public:
  ParamChangeEvent() = default;
  explicit ParamChangeEvent(
      double startTime,
      double endTime,
      float startValue,
      float endValue,
      std::function<float(double, double, float, float, double)> &&calculateValue,
      ParamChangeEventType type);

  ParamChangeEvent(const ParamChangeEvent &other) = delete;
  ParamChangeEvent& operator=(const ParamChangeEvent &other) = delete;

  explicit ParamChangeEvent(ParamChangeEvent &&other) noexcept
    : startTime_(other.startTime_),
      endTime_(other.endTime_),
      calculateValue_(std::move(other.calculateValue_)),
      startValue_(other.startValue_),
      endValue_(other.endValue_),
      type_(other.type_) {}
  ParamChangeEvent& operator=(ParamChangeEvent &&other) noexcept {
    if (this != &other) {
      startTime_ = other.startTime_;
      endTime_ = other.endTime_;
      calculateValue_ = std::move(other.calculateValue_);
      startValue_ = other.startValue_;
      endValue_ = other.endValue_;
      type_ = other.type_;
    }
    return *this;
  }

  [[nodiscard]] inline double getEndTime() const noexcept {
    return endTime_;
  }
  [[nodiscard]] inline double getStartTime() const noexcept {
    return startTime_;
  }
  [[nodiscard]] inline float getEndValue() const noexcept {
    return endValue_;
  }
  [[nodiscard]] inline float getStartValue() const noexcept {
    return startValue_;
  }
  [[nodiscard]] inline const std::function<float(double, double, float, float, double)>&
  getCalculateValue() const noexcept {
    return calculateValue_;
  }
  [[nodiscard]] inline ParamChangeEventType getType() const noexcept {
    return type_;
  }

  inline void setEndTime(double endTime) noexcept {
    endTime_ = endTime;
  }
  inline void setStartValue(float startValue) noexcept {
    startValue_ = startValue;
  }
  inline void setEndValue(float endValue) noexcept {
    endValue_ = endValue;
  }

 private:
  double startTime_;
  double endTime_;
  std::function<float(double, double, float, float, double)> calculateValue_;
  float startValue_;
  float endValue_;
  ParamChangeEventType type_;
};

} // namespace audioapi
