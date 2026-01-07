#pragma once

#include <functional>
#include <new>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

struct NoneType {};
inline constexpr NoneType None{};

/// @brief A Result type that can represent either a success (Ok) or an error (Err).
/// @tparam T value type for success
/// @tparam E error type for failure
/// @note Specializations for void T and/or void E are not provided. Use NoneType instead.
/// Design inspired by Rust's Result type.
/// https://doc.rust-lang.org/std/result/
///
/// @example
/// /// Creating an Ok Result and mapping its error:
/// Result<int, std::string> res = Result<int, int>::Err(404)
///   .map_err<std::string>([](auto code){
///      return "Error code: " + std::to_string(code);
///    });
///
template <typename T = NoneType, typename E = NoneType>
class Result {
  struct OkTag {};
  struct ErrTag {};

  explicit Result(OkTag, const T &value) : ok_value(value), is_ok_(true) {}
  explicit Result(OkTag, T &&value) : ok_value(std::move(value)), is_ok_(true) {}
  explicit Result(ErrTag, const E &error) : err_value(error), is_ok_(false) {}
  explicit Result(ErrTag, E &&error) : err_value(std::move(error)), is_ok_(false) {}

 public:
  Result(const Result<T, E> &other) {
    is_ok_ = other.is_ok_;
    if (is_ok_) {
      new (&ok_value) T(other.ok_value);
    } else {
      new (&err_value) E(other.err_value);
    }
  }

  Result(Result<T, E> &&other) noexcept(
      std::is_nothrow_move_constructible_v<T> && std::is_nothrow_move_constructible_v<E>) {
    is_ok_ = other.is_ok_;
    if (is_ok_) {
      new (&ok_value) T(std::move(other.ok_value));
    } else {
      new (&err_value) E(std::move(other.err_value));
    }
  }

  Result &operator=(const Result &other) {
    if (this == &other)
      return *this;
    if (is_ok_ == other.is_ok_) {
      if (is_ok_)
        ok_value = other.ok_value;
      else
        err_value = other.err_value;
    } else {
      if (is_ok_) {
        ok_value.~T();
        new (&err_value) E(other.err_value);
        is_ok_ = false;
      } else {
        err_value.~E();
        new (&ok_value) T(other.ok_value);
        is_ok_ = true;
      }
    }
    return *this;
  }

  Result &operator=(Result &&other) noexcept(
      std::is_nothrow_move_assignable_v<T> && std::is_nothrow_move_assignable_v<E> &&
      std::is_nothrow_move_constructible_v<T> && std::is_nothrow_move_constructible_v<E>) {
    if (this == &other)
      return *this;
    if (is_ok_ == other.is_ok_) {
      if (is_ok_)
        ok_value = std::move(other.ok_value);
      else
        err_value = std::move(other.err_value);
    } else {
      if (is_ok_) {
        ok_value.~T();
        new (&err_value) E(std::move(other.err_value));
        is_ok_ = false;
      } else {
        err_value.~E();
        new (&ok_value) T(std::move(other.ok_value));
        is_ok_ = true;
      }
    }
    return *this;
  }

  ~Result() {
    if (is_ok_) {
      ok_value.~T();
    } else {
      err_value.~E();
    }
  }

  /// @brief Creates a success Result.
  static Result<T, E> Ok(const T &value) {
    return Result<T, E>(OkTag{}, value);
  }

  /// @brief Creates a success Result from an rvalue.
  static Result<T, E> Ok(T &&value) {
    return Result<T, E>(OkTag{}, std::move(value));
  }

  /// @brief Creates an error Result.
  static Result<T, E> Err(const E &error) {
    return Result<T, E>(ErrTag{}, error);
  }

  /// @brief Creates an error Result from an rvalue.
  static Result<T, E> Err(E &&error) {
    return Result<T, E>(ErrTag{}, std::move(error));
  }

  /// @brief Returns true if the result is Ok.
  [[nodiscard]] bool is_ok() const {
    return is_ok_;
  }

  /// @brief Returns true if the result is Err.
  [[nodiscard]] bool is_err() const {
    return !is_ok_;
  }

  /// @brief Returns the contained Ok value, consuming the Result.
  /// @throws std::runtime_error if the value is an Err.
  [[nodiscard]] T expect(const std::string &msg) && {
    if (!is_ok_) {
      throw std::runtime_error(msg);
    }
    return std::move(ok_value);
  }

  /// @brief Returns the contained Ok value.
  /// @throws std::runtime_error if the value is an Err.
  [[nodiscard]] const T &expect(const std::string &msg) const & {
    if (!is_ok_) {
      throw std::runtime_error(msg);
    }
    return ok_value;
  }

  /// @brief Returns the contained Ok value, consuming the Result.
  /// @throws std::runtime_error if the value is an Err.
  [[nodiscard]] T unwrap() && {
    if (!is_ok_) {
      throw std::runtime_error("Called unwrap on an Err value");
    }
    return std::move(ok_value);
  }

  /// @brief Returns the contained Ok value.
  /// @throws std::runtime_error if the value is an Err.
  [[nodiscard]] const T &unwrap() const & {
    if (!is_ok_) {
      throw std::runtime_error("Called unwrap on an Err value");
    }
    return ok_value;
  }

  /// @brief Returns the contained Ok value or a default. Consumes self.
  [[nodiscard]] T unwrap_or(T &&default_value) && {
    if (is_ok_) {
      return std::move(ok_value);
    }
    return std::move(default_value);
  }

  /// @brief Returns the contained Ok value or computes it from a closure. Consumes self.
  [[nodiscard]] T unwrap_or_else(const std::function<T(E &&)> &func) && {
    if (is_ok_) {
      return std::move(ok_value);
    }
    return func(std::move(err_value));
  }

  /// @brief Returns the contained Ok value without checking. UB if Err.
  [[nodiscard]] T unwrap_unchecked() && noexcept {
    return std::move(ok_value);
  }

  /// @brief Returns the contained Ok value without checking. UB if Err.
  [[nodiscard]] const T &unwrap_unchecked() const & noexcept {
    return ok_value;
  }

  /// @brief Returns the contained Err value, consuming the Result.
  /// @throws std::runtime_error if the value is Ok.
  [[nodiscard]] E unwrap_err() && {
    if (is_ok_) {
      throw std::runtime_error("Called unwrap_err on an Ok value");
    }
    return std::move(err_value);
  }

  /// @brief Returns the contained Err value.
  /// @throws std::runtime_error if the value is Ok.
  [[nodiscard]] const E &unwrap_err() const & {
    if (is_ok_) {
      throw std::runtime_error("Called unwrap_err on an Ok value");
    }
    return err_value;
  }

  /// @brief Returns the contained Err value, consuming the Result.
  /// @throws std::runtime_error if the value is Ok.
  [[nodiscard]] E expect_err(const std::string &msg) && {
    if (is_ok_) {
      throw std::runtime_error(msg);
    }
    return std::move(err_value);
  }

  /// @brief Returns the contained Err value.
  /// @throws std::runtime_error if the value is Ok.
  [[nodiscard]] const E &expect_err(const std::string &msg) const & {
    if (is_ok_) {
      throw std::runtime_error(msg);
    }
    return err_value;
  }

  /// @brief Returns the contained Err value without checking. UB if Ok.
  [[nodiscard]] E unwrap_err_unchecked() && noexcept {
    return std::move(err_value);
  }

  /// @brief Returns the contained Err value without checking. UB if Ok.
  [[nodiscard]] const E &unwrap_err_unchecked() const & noexcept {
    return err_value;
  }

  /// @brief Maps a Result<T, E> to Result<U, E> by applying a function to a contained Ok value.
  template <typename F>
  [[nodiscard]] auto map(F &&ok_func) && {
    using U = std::invoke_result_t<F, T>;
    using ResultType = Result<U, E>;
    if (is_ok_) {
      return ResultType::Ok(ok_func(std::move(ok_value)));
    } else {
      return ResultType::Err(std::move(err_value));
    }
  }

  /// @brief Maps a Result<T, E> to Result<T, F> by applying a function to a contained Err value.
  template <typename F>
  [[nodiscard]] auto map_err(F &&err_func) && {
    using U = std::invoke_result_t<F, E>;
    using ResultType = Result<T, U>;
    if (is_ok_) {
      return ResultType::Ok(std::move(ok_value));
    } else {
      return ResultType::Err(err_func(std::move(err_value)));
    }
  }

  /// @brief Returns the provided default (if Err), or applies a function to the contained value (if Ok).
  template <typename F, typename U>
  [[nodiscard]] auto map_or(F &&ok_func, U &&default_value) && {
    if (is_ok_) {
      return ok_func(std::move(ok_value));
    } else {
      return std::forward<U>(default_value);
    }
  }

  /// @brief Maps a Result<T, E> to U by applying fallback function default to a contained Err value, or function f to a contained Ok value.
  template <typename FT, typename FE>
  [[nodiscard]] auto map_or_else(FT &&ok_func, FE &&err_func) && {
    if (is_ok_) {
      return ok_func(std::move(ok_value));
    } else {
      return err_func(std::move(err_value));
    }
  }

  /// @brief Calls the provided closure with the contained Ok value, if any, otherwise returns the Err value of self.
  /// @tparam for Result<T,E> F is a closure that takes T and returns Result<TNew, E>
  template <typename F>
  [[nodiscard]] auto and_then(F &&ok_func) && {
    using ResultType = std::invoke_result_t<F, T>;
    if (is_ok_) {
      return ok_func(std::move(ok_value));
    } else {
      return ResultType::Err(std::move(err_value));
    }
  }

  /// @brief Calls the provided closure with the contained Err value, if any, otherwise returns the Ok value of self.
  /// @tparam for Result<T,E> F is a closure that takes E and returns Result<T, ENew>
  template <typename F>
  [[nodiscard]] auto or_else(F &&err_func) && {
    using ResultType = std::invoke_result_t<F, E>;
    if (is_ok_) {
      return ResultType::Ok(std::move(ok_value));
    } else {
      return err_func(std::move(err_value));
    }
  }

 private:
  union {
    T ok_value;
    E err_value;
  };
  bool is_ok_;
};
