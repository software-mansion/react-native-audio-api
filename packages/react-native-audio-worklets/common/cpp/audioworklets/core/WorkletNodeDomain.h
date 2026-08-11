#pragma once

#include <cstdint>

namespace audioworklets {

enum class WorkletNodeDomain : std::uint8_t {
  TimeDomain = 0,
  FrequencyDomain = 1,
};

} // namespace audioworklets
