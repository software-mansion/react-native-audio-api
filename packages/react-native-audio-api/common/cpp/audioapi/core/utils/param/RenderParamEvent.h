#pragma once

#include <audioapi/core/utils/param/ParamEvent.h>

#include <audioapi/core/types/ParamEventType.h>
#include <functional>
#include <utility>

namespace audioapi {

/// @brief A RenderParamEvent extends ParamEvent with additional properties and a value calculation
/// function that can compute the parameter value at any time during the event's active period
/// based on its type and the current state of the queue.
///
/// Times exist in two forms. The inherited startTime/endTime are snapped to the
/// sample-frame grid by ParamRenderQueue::push and drive ordering and
/// effect-boundary decisions, so an event scheduled at T takes effect exactly
/// at frame round(T * sampleRate) — the convention Blink and the WPT reference
/// use. rawStartTime/rawEndTime keep the times as scheduled and feed the value
/// interpolation, which the spec defines on real times: a ramp between two
/// times inside one frame must still interpolate on those times, not on their
/// collapsed snapped values.
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
        rawStartTime_(startTime),
        rawEndTime_(endTime),
        startValue_(startValue),
        endValue_(endValue) {}

  RenderParamEvent(const RenderParamEvent &) = delete;
  RenderParamEvent &operator=(const RenderParamEvent &) = delete;

  RenderParamEvent(RenderParamEvent &&other) noexcept
      : ParamEvent(std::move(other)),
        calculateValue_(std::move(other.calculateValue_)),
        rawStartTime_(other.rawStartTime_),
        rawEndTime_(other.rawEndTime_),
        startValue_(other.startValue_),
        endValue_(other.endValue_) {}

  RenderParamEvent &operator=(RenderParamEvent &&other) noexcept {
    if (this != &other) {
      ParamEvent::operator=(std::move(other));
      calculateValue_ = std::move(other.calculateValue_);
      rawStartTime_ = other.rawStartTime_;
      rawEndTime_ = other.rawEndTime_;
      startValue_ = other.startValue_;
      endValue_ = other.endValue_;
    }
    return *this;
  }

  [[nodiscard]] double getRawStartTime() const noexcept {
    return rawStartTime_;
  }

  [[nodiscard]] double getRawEndTime() const noexcept {
    return rawEndTime_;
  }

  void setRawStartTime(double rawStartTime) noexcept {
    rawStartTime_ = rawStartTime;
  }

  void setRawEndTime(double rawEndTime) noexcept {
    rawEndTime_ = rawEndTime;
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
  double rawStartTime_ = 0.0;
  double rawEndTime_ = 0.0;
  float startValue_ = 0.0f;
  float endValue_ = 0.0f;
};

} // namespace audioapi
