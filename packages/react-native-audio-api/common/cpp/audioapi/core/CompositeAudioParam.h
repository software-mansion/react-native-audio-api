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

// Also accept function references (NTTP decay).
template <typename R, typename... Args>
struct CombineTraits<R (&)(Args...)> : CombineTraits<R (*)(Args...)> {};

} // namespace detail

template <auto CombineFunction>
concept ValidCombineFunction =
    requires { typename detail::CombineTraits<decltype(CombineFunction)>; };

/// @brief A composite (computed) audio param — a pure function of its children.
///
/// Directly corresponds to a spec @c computedValue formula (e.g.
/// @c computedOscFrequency, @c computedFrequency, @c computedPlaybackRate). The
/// combine function is the only template argument; its arity and float-only
/// signature are deduced from its type, so child types are not repeated at the
/// declaration site. The function is captureless, so it cannot reach into param
/// internals. Children are always simple @c AudioParam s — no composites of
/// composites.
///
/// The composite is not JS-connectable, so it has no @c inputBuffer_ of its
/// own. @c processXRateParam processes each child (idempotent — safe if already
/// processed), applies @c CombineFunction, clamps to the composite's nominal
/// range, and caches the result. @c CombineFunction is inlined and the process
/// methods are statically resolved — no dispatch overhead on the audio thread.
template <auto CombineFunction>
  requires ValidCombineFunction<CombineFunction>
class CompositeAudioParam : public GeneralizedAudioParam {
 public:
  static constexpr std::size_t kArity = detail::CombineTraits<decltype(CombineFunction)>::arity;

  // Fixed args first so the child pack does not swallow min/max/context.
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

    // Process each child (idempotent — cache hit if already processed) and combine.
    const float raw = [&]<std::size_t... I>(std::index_sequence<I...>) {
      return CombineFunction(children_[I]->processKRateParam(time)...);
    }(std::make_index_sequence<kArity>{});

    return finalizeKRate(raw, time);
  }

  std::shared_ptr<DSPAudioBuffer> processARateParam(int framesToProcess, double time) final {
    if (aRateCacheHit(framesToProcess, time)) {
      return outputBuffer_;
    }

    // Render each child into its own buffer (idempotent) and gather the spans.
    // The child buffers stay alive because each child owns its own outputBuffer_.
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
