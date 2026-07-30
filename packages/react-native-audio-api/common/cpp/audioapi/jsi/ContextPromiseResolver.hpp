#pragma once

#include <concepts>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>

#include <audioapi/utils/AudioBuffer.hpp>

namespace audioapi {

class Promise;
class BaseAudioContext;
enum class ContextState : std::uint8_t;

template <typename T>
class ContextPromiseResolver;

using OfflineAudioContextResultPromise = ContextPromiseResolver<std::shared_ptr<AudioBuffer>>;

namespace detail {

template <typename T>
struct ContextPromiseResolveFn {
  using type = std::function<void(const T &)>;
};

template <>
struct ContextPromiseResolveFn<void> {
  using type = std::function<void()>;
};

struct ContextPromiseNoValue {};

template <typename T>
using ContextPromiseResolveValue =
    std::conditional_t<std::is_same_v<T, void>, ContextPromiseNoValue, T>;

} // namespace detail

/// Lifecycle / rendering promise bridge to JSI.
/// @tparam T Value type resolved into the promise, or `void` for lifecycle ops.
template <typename T>
class ContextPromiseResolver {
 public:
  ContextPromiseResolver(
      std::function<void()> resolve,
      std::function<void(const std::string &)> reject)
    requires std::same_as<T, void>
      : resolve_(std::move(resolve)), reject_(std::move(reject)) {}

  ContextPromiseResolver(
      std::function<void(const detail::ContextPromiseResolveValue<T> &)> resolve,
      std::function<void(const std::string &)> reject)
    requires(!std::same_as<T, void>)
      : resolve_(std::move(resolve)), reject_(std::move(reject)) {}

  void resolve() const
    requires std::same_as<T, void>
  {
    resolve_();
  }

  void resolve(const detail::ContextPromiseResolveValue<T> &value) const
    requires(!std::same_as<T, void>)
  {
    resolve_(value);
  }

  static void resolve(const std::shared_ptr<ContextPromiseResolver<T>> &promise)
    requires std::same_as<T, void>
  {
    if (promise != nullptr) {
      promise->resolve();
    }
  }

  static void resolve(
      const std::shared_ptr<ContextPromiseResolver<T>> &promise,
      const detail::ContextPromiseResolveValue<T> &value)
    requires(!std::same_as<T, void>)
  {
    if (promise != nullptr) {
      promise->resolve(value);
    }
  }

  void reject(const std::string &message) const {
    reject_(message);
  }

  static void reject(
      const std::shared_ptr<ContextPromiseResolver<T>> &promise,
      const std::string &message) {
    if (promise != nullptr) {
      promise->reject(message);
    }
  }

  static std::shared_ptr<ContextPromiseResolver<void>> makeContextPromiseResolver(
      Promise &&promise,
      const std::shared_ptr<BaseAudioContext> &audioContext,
      ContextState nextState)
    requires std::same_as<T, void>;

  static std::shared_ptr<OfflineAudioContextResultPromise> makeOfflineAudioContextResultResolver(
      Promise &&promise,
      const std::shared_ptr<BaseAudioContext> &audioContext)
    requires(!std::same_as<T, void>);

 private:
  detail::ContextPromiseResolveFn<T>::type resolve_;
  std::function<void(const std::string &)> reject_;
};

} // namespace audioapi
