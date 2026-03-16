#pragma once

#include <cstdint>

namespace audioapi {

enum class ParamChangeEventType : std::uint8_t {
  LINEAR_RAMP,
  EXPONENTIAL_RAMP,
  SET_VALUE,
  SET_TARGET,
  SET_VALUE_CURVE,
};

} // namespace audioapi
