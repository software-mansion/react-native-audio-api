#pragma once

#include <audioapi/core/AudioParam.h>
#include <audioapi/core/GeneralizedAudioParam.h>
#include <audioapi/utils/AudioArray.hpp>
#include <audioapi/utils/AudioBuffer.hpp>

#include <array>
#include <concepts>
#include <cstddef>
#include <memory>
#include <span>
#include <utility>

namespace audioapi {

namespace detail {

template <typename F>
struct CombineTraits;

template <typename R, typename... Args>
struct CombineTraits<R (*)(Args...)> {
  static_assert(std::same_as<R, float>, "combine function must return float");
  static_assert((std::same_as<Args, float> && ...), "combine function arguments must all be float");
  static constexpr std::size_t arity = sizeof...(Args);
};

// A function used as a non-type template argument can retain either pointer or
// reference type; both forms must use the same signature validation.
template <typename R, typename... Args>
struct CombineTraits<R (&)(Args...)> : CombineTraits<R (*)(Args...)> {};

} // namespace detail

template <auto CombineFunction>
concept ValidCombineFunction =
    requires { typename detail::CombineTraits<decltype(CombineFunction)>; };

/// @brief Combines child parameters into a spec-defined computed value.
///
/// Directly corresponds to a spec @c computedValue formula (e.g.
/// @c computedOscFrequency, @c computedFrequency, @c computedPlaybackRate). The
/// combine function is the only template argument; its arity and float-only
/// signature are deduced from its type, so child types are not repeated at the
/// declaration site. The function is captureless, so it cannot reach into param
/// internals. Children are always simple @c AudioParam s — no composites of
/// composites.
///
/// The composite is internal rather than JavaScript-connectable, so it has no
/// modulation input of its own. Processing a child with the same cache key used
/// by another consumer reuses that child's result instead of consuming its
/// modulation twice. The combined result is then clamped and cached against the
/// composite's own nominal range.
template <auto CombineFunction>
  requires ValidCombineFunction<CombineFunction>
class CompositeAudioParam : public GeneralizedAudioParam {
 public:
  static constexpr std::size_t kArity = detail::CombineTraits<decltype(CombineFunction)>::arity;

  template <typename... Children>
    requires(sizeof...(Children) == kArity) &&
                (std::convertible_to<Children, std::shared_ptr<AudioParam>> && ...)
  explicit CompositeAudioParam(
      float minValue,
      float maxValue,
      const std::shared_ptr<BaseAudioContext> &context,
      Children... children)
      : GeneralizedAudioParam(minValue, maxValue, context), children_{std::move(children)...} {}

  float processKRateParam(double time) final {
    if (kRateCacheHit(time)) {
      return cachedKRateValue_;
    }

    const float raw = [&]<std::size_t... I>(std::index_sequence<I...>) {
      return CombineFunction(children_[I]->processKRateParam(time)...);
    }(std::make_index_sequence<kArity>{});

    return finalizeKRate(raw, time);
  }

  std::shared_ptr<DSPAudioBuffer> processARateParam(int framesToProcess, double time) final {
    if (aRateCacheHit(framesToProcess, time)) {
      return outputBuffer_;
    }

    // Each span borrows storage owned by its corresponding child, which outlives this call.
    const std::array<std::span<const float>, kArity> childSpans =
        [&]<std::size_t... I>(std::index_sequence<I...>) {
          return std::array<std::span<const float>, kArity>{
              children_[I]->processARateParam(framesToProcess, time)->getChannel(0)->span()...};
        }(std::make_index_sequence<kArity>{});

    auto outputData = outputBuffer_->getChannel(0)->span();
    for (int i = 0; i < framesToProcess; ++i) {
      const auto frame = static_cast<std::size_t>(i);
      outputData[frame] = [&]<std::size_t... I>(std::index_sequence<I...>) {
        return CombineFunction(childSpans[I][frame]...);
      }(std::make_index_sequence<kArity>{});
    }

    finalizeARate(framesToProcess, time);
    return outputBuffer_;
  }

 private:
  std::array<std::shared_ptr<AudioParam>, kArity> children_;
};

} // namespace audioapi
