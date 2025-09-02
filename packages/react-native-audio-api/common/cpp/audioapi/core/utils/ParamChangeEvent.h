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

  [[nodiscard]] inline double getEndTime() const {
    return endTime_;
  }
  [[nodiscard]] inline double getStartTime() const {
    return startTime_;
  }
  [[nodiscard]] inline float getEndValue() const {
    return endValue_;
  }
  [[nodiscard]] inline float getStartValue() const {
    return startValue_;
  }
  [[nodiscard]] inline std::function<float(double, double, float, float, double)>
  getCalculateValue() const {
    return calculateValue_;
  }
  [[nodiscard]] inline ParamChangeEventType getType() const {
    return type_;
  }

  inline void setEndTime(double endTime) {
    endTime_ = endTime;
  }
  inline void setStartValue(float startValue) {
    startValue_ = startValue;
  }
  inline void setEndValue(float endValue) {
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
