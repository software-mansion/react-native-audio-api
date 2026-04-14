#pragma once
#include <functional>
#include <memory>
#include <utility>

namespace audioapi {

/// @brief A forward declaration of a move-only function wrapper.
/// @note it is somehow required to have <R(Args...)> specialization below
/// @tparam Signature
template <typename Signature>
class move_only_function; // Forward declaration

/// @brief A move-only function wrapper similar to std::function but non-copyable.
/// @details This class allows you to store and invoke callable objects (like lambdas, function pointers, or functors)
/// that can be moved but not copied. It is useful for scenarios where you want to ensure that the callable
/// is unique and cannot be duplicated.
/// @note This implementation uses type erasure to store the callable object.
/// @note The callable object must be invocable with the specified arguments and return type.
/// @note IMPORTANT: This thing is implemented in C++23 standard and can be replaced with std::move_only_function once we switch to C++23.
/// @tparam R
/// @tparam ...Args
template <typename R, typename... Args>
class move_only_function<R(Args...)> {
  /// @brief The base class for type erasure.
  /// @note It gets optimized by Empty Base Optimization (EBO) when possible.
  struct callable_base {
    virtual ~callable_base() = default;
    virtual R operator()(Args... args) = 0;
  };

  /// @brief The implementation of the callable object.
  /// @tparam F
  template <typename F>
  struct callable_impl : callable_base {
    /// @brief The stored callable object.
    F f;

    /// @brief Construct a new callable_impl object.
    /// @tparam G
    /// @param func
    /// @note The enable_if_t ensures that F can be constructed from G&&.
    template <typename G, typename = std::enable_if_t<std::is_constructible_v<F, G &&>>>
    explicit callable_impl(G &&func) : f(std::forward<G>(func)) {}

    /// @brief Invoke the stored callable object with the given arguments.
    /// @param args
    /// @return R
    /// @note The if constexpr is used to handle the case when R is void.
    inline R operator()(Args... args) override {
      if constexpr (std::is_void_v<R>) {
        /// To avoid "warning: expression result unused" when R is void
        f(std::forward<Args>(args)...);
      } else {
        return f(std::forward<Args>(args)...);
      }
    }
  };

  /// @brief The unique pointer to the base callable type.
  std::unique_ptr<callable_base> impl_;

 public:
  move_only_function() = default;
  move_only_function(std::nullptr_t) noexcept : impl_(nullptr) {}

  template <typename F>
  move_only_function(F &&f)
      : impl_(std::make_unique<callable_impl<std::decay_t<F>>>(std::forward<F>(f))) {}

  move_only_function(const move_only_function &) = delete;
  move_only_function &operator=(const move_only_function &) = delete;

  move_only_function(move_only_function &&) = default;
  move_only_function &operator=(move_only_function &&) = default;

  inline explicit operator bool() const noexcept {
    return impl_ != nullptr;
  }

  inline R operator()(Args... args) {
    /// We are unlikely to hit this case as we want to optimize for the common case.
    if (impl_ == nullptr) [[unlikely]] {
      throw std::bad_function_call{};
    }
    return (*impl_)(std::forward<Args>(args)...);
  }

  void swap(move_only_function &other) noexcept {
    impl_.swap(other.impl_);
  }
};
} // namespace audioapi
