#pragma once

#include <audioapi/compatibility/StableAPI.h>

#include <cstddef>

namespace audioworklets {

struct WorkletNodeOptions {
  size_t bufferLength = 1024;
  float smoothingTimeConstant = audioapi::AnalyserOptions::kDefaultSmoothingTimeConstant;
};

} // namespace audioworklets
