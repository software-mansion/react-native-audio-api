#pragma once

namespace audioapi {

enum class AutomationEventType {
  LINEAR_RAMP,
  EXPONENTIAL_RAMP,
  SET_VALUE,
  SET_TARGET,
  SET_VALUE_CURVE,
};

} // namespace audioapi
