#pragma once

#include <audioapi/core/utils/param/RenderParamEvent.h>
#include <audioapi/dsp/AudioUtils.hpp>
#include <audioapi/utils/AudioArray.hpp>
#include <memory>
#include <utility>

namespace audioapi {

/// @brief A factory for creating RenderParamEvents and resolving their values
/// based on the current state of the queue.
class ParamRenderEventFactory {
 public:
  static RenderParamEvent createSetValueEvent(float value, double startTime) {
    auto calculateValue =
        [](double startTime, double /* endTime */, float startValue, float endValue, double time) {
          if (time < startTime) {
            return startValue;
          }

          return endValue;
        };

    return RenderParamEvent(
        startTime, startTime, value, value, std::move(calculateValue), ParamEventType::SET_VALUE);
  }

  static RenderParamEvent createLinearRampEvent(float value, double endTime) {
    auto calculateValue =
        [](double startTime, double endTime, float startValue, float endValue, double time) {
          if (time < startTime) {
            return startValue;
          }

          if (time < endTime) {
            return static_cast<float>(
                startValue + (endValue - startValue) * (time - startTime) / (endTime - startTime));
          }

          return endValue;
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

          if (time < startTime) {
            return startValue;
          }

          if (time < endTime) {
            return static_cast<float>(
                startValue *
                pow(endValue / startValue, (time - startTime) / (endTime - startTime)));
          }

          return endValue;
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

      if (time < startTime) {
        return startValue;
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
          if (time < startTime) {
            return startValue;
          }

          if (time < endTime) {
            // Calculate position in the array based on time progress
            auto k = static_cast<int>(std::floor(
                static_cast<double>(length - 1) / (endTime - startTime) * (time - startTime)));
            // Calculate interpolation factor between adjacent array elements
            auto factor = static_cast<float>(
                (time - startTime) * static_cast<double>(length - 1) / (endTime - startTime) - k);
            return dsp::linearInterpolate(values->span(), k, k + 1, factor);
          }

          return endValue;
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
