#pragma once

#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/utils/Constants.h>
#include <audioapi/utils/AudioArray.hpp>
#include <audioapi/utils/AudioBuffer.hpp>

#include <algorithm>
#include <memory>
#include <optional>
#include <utility>

namespace audioapi {

/// @brief Common interface for all audio params.
///
/// Enforces the shared @c processKRateParam / @c processARateParam contract and
/// owns the nominal range, the a-rate output buffer, and the per-quantum
/// memoization state. Clamping to <tt>[minValue_, maxValue_]</tt> and caching
/// happen through @c finalizeKRate / @c finalizeARate so subclasses never
/// duplicate that logic.
///
/// @note Idempotent process: each param remembers the arguments it was last
/// processed for (@c time for k-rate; <tt>(framesToProcess, time)</tt> for
/// a-rate). A repeated call with the same arguments returns the cached result
/// without re-consuming modulation, so callers need not coordinate who
/// processes whom within a render quantum.
class GeneralizedAudioParam {
 public:
  explicit GeneralizedAudioParam(
      float minValue,
      float maxValue,
      const std::shared_ptr<BaseAudioContext> &context);

  virtual ~GeneralizedAudioParam() = default;

  [[nodiscard]] float getMinValue() const noexcept {
    return minValue_;
  }

  [[nodiscard]] float getMaxValue() const noexcept {
    return maxValue_;
  }

  /// @note JS Thread only
  [[nodiscard]] double getCurrentTime() const noexcept {
    if (auto context = context_.lock()) {
      return context->getCurrentTime();
    }
    return 0.0;
  }

  /// @note Audio Thread only
  /// @brief Materialize (or return cached) this quantum's k-rate value.
  virtual float processKRateParam(double time) = 0;

  /// @note Audio Thread only
  /// @brief Materialize (or return cached) this quantum's a-rate values in @c outputBuffer_.
  virtual std::shared_ptr<DSPAudioBuffer> processARateParam(int framesToProcess, double time) = 0;

 protected:
  /// @brief True if the k-rate value for @p time is already cached.
  [[nodiscard]] bool kRateCacheHit(double time) const noexcept {
    return processedKRateTime_.has_value() && processedKRateTime_.value() == time;
  }

  /// @brief True if the a-rate values for (@p framesToProcess, @p time) are already in @c outputBuffer_.
  [[nodiscard]] bool aRateCacheHit(int framesToProcess, double time) const noexcept {
    return processedARateKey_.has_value() && processedARateKey_->first == framesToProcess &&
        processedARateKey_->second == time;
  }

  [[nodiscard]] float clampToNominalRange(float raw) const noexcept {
    return std::clamp(raw, minValue_, maxValue_);
  }

  /// @brief Clamp @p raw to the nominal range, cache it, record the k-rate key, and return it.
  float finalizeKRate(float raw, double time) noexcept {
    cachedKRateValue_ = clampToNominalRange(raw);
    processedKRateTime_ = time;
    return cachedKRateValue_;
  }

  /// @brief Clamp the first @p framesToProcess samples of @c outputBuffer_ to the nominal
  /// range and record the a-rate key.
  void finalizeARate(int framesToProcess, double time);

  std::weak_ptr<BaseAudioContext> context_;
  float minValue_;
  float maxValue_;
  float cachedKRateValue_ = 0.0f;

  // Identity of the last successful process (k-rate / a-rate tracked separately).
  std::optional<double> processedKRateTime_;
  std::optional<std::pair<int, double>> processedARateKey_;

  // Output buffer for a-rate processing.
  std::shared_ptr<DSPAudioBuffer> outputBuffer_;
};

} // namespace audioapi
