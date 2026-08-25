#pragma once

#include <audioapi/core/utils/param/RenderParamEvent.h>
#include <audioapi/dsp/AudioUtils.h>
#include <audioapi/utils/AudioArray.hpp>
#include <algorithm>
#include <cstddef>
#include <memory>
#include <utility>

namespace audioapi {

/// @brief A factory for creating RenderParamEvents and resolving their values
/// based on the current state of the queue.
///
/// The calculateValue functions are pure interpolation formulas evaluated on an
/// event's RAW times; whether an event is in effect at a given frame is decided
/// by ParamRenderQueue::computeValueAtTime against the snapped times. Because a
/// frame's exact time can fall fractionally outside [rawStartTime, rawEndTime)
/// while the frame still belongs to the event, each formula must extrapolate
/// gracefully (clamp, or accept a sub-ULP overshoot) instead of branching on
/// the raw boundary.
class ParamRenderEventFactory {
 public:
  static RenderParamEvent createSetValueEvent(float value, double startTime) {
    auto calculateValue = [](double /* startTime */,
                             double /* endTime */,
                             float /* startValue */,
                             float endValue,
                             double /* time */) {
      return endValue;
    };

    return RenderParamEvent(
        startTime, startTime, value, value, std::move(calculateValue), ParamEventType::SET_VALUE);
  }

  static RenderParamEvent createLinearRampEvent(float value, double endTime) {
    auto calculateValue =
        [](double startTime, double endTime, float startValue, float endValue, double time) {
          if (endTime <= startTime) {
            return endValue;
          }

          return static_cast<float>(
              startValue + (endValue - startValue) * (time - startTime) / (endTime - startTime));
        };

    return RenderParamEvent(
        0.0, endTime, 0.0f, value, std::move(calculateValue), ParamEventType::LINEAR_RAMP);
  }

  static RenderParamEvent createExponentialRampEvent(float value, double endTime) {
    auto calculateValue =
        [](double startTime, double endTime, float startValue, float endValue, double time) {
          if (startValue * endValue < 0 || startValue == 0) {
            return startValue;
          }

          if (endTime <= startTime) {
            return endValue;
          }

          return static_cast<float>(
              startValue * pow(endValue / startValue, (time - startTime) / (endTime - startTime)));
        };

    return RenderParamEvent(
        0.0, endTime, 0.0f, value, std::move(calculateValue), ParamEventType::EXPONENTIAL_RAMP);
  }

  static RenderParamEvent
  createSetTargetEvent(float target, double startTime, double timeConstant) {
    auto calculateValue = [timeConstant, target](
                              double startTime,
                              double /* endTime */,
                              float startValue,
                              float /* endValue */,
                              double time) {
      if (timeConstant == 0) {
        return target;
      }

      return static_cast<float>(
          target + (startValue - target) * exp(-(time - startTime) / timeConstant));
    };

    return RenderParamEvent(
        startTime,
        startTime, // SetTarget events have infinite duration conceptually
        0.0f,
        0.0f, // End value is not meaningful for infinite events
        std::move(calculateValue),
        ParamEventType::SET_TARGET);
  }

  static RenderParamEvent createSetValueCurveEvent(
      const std::shared_ptr<AudioArray> &values,
      size_t length,
      double startTime,
      double duration) {
    auto calculateValue =
        [values, length](
            double startTime, double endTime, float startValue, float endValue, double time) {
          if (endTime <= startTime) {
            return endValue;
          }

          // Position in the array based on time progress, clamped so a frame
          // snapped fractionally outside the raw interval stays in range.
          double position = std::clamp(
              static_cast<double>(length - 1) / (endTime - startTime) * (time - startTime),
              0.0,
              static_cast<double>(length - 1));
          auto k = static_cast<size_t>(position);
          size_t nextIndex = std::min(k + 1, length - 1);
          auto factor = static_cast<float>(position - static_cast<double>(k));
          return dsp::linearInterpolate(values->span(), k, nextIndex, factor);
        };

    return RenderParamEvent(
        startTime,
        startTime + duration,
        0.0f,
        values->span()[length - 1],
        std::move(calculateValue),
        ParamEventType::SET_VALUE_CURVE);
  }
};

} // namespace audioapi
