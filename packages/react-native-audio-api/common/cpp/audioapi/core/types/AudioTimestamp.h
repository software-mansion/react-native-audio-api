#pragma once

namespace audioapi {

/// Web Audio `AudioTimestamp`: one moment expressed on the context clock and on the
/// `performance.now()` clock.
struct AudioTimestamp {
  /// Seconds, on the context's `currentTime` axis.
  double contextTime = 0.0;
  /// Milliseconds, on the `performance.now()` axis (`std::chrono::steady_clock`).
  double performanceTime = 0.0;
};

} // namespace audioapi
