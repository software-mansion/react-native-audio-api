#pragma once
#include <array>
#include <concepts>
#include <cstddef>
#include <cstring>
#include <functional>
#include <new>
#include <type_traits>
#include <utility>

namespace audioapi {

template <size_t N, typename _Callable, typename _FpReturnType, typename... _FpArgs>
concept CallableConcept =
    (sizeof(std::decay_t<_Callable>) <= N) && requires(_Callable &&_c, _FpArgs &&..._args) {
      { _c(std::forward<_FpArgs>(_args)...) } -> std::convertible_to<_FpReturnType>;
    };

template <size_t N, typename _Fp>
class FatFunction;

/// @brief FatFunction is a fixed-size function wrapper that can store callable objects
///        of a specific size N without dynamic memory allocation.
/// @tparam N Size in bytes to allocate for the callable object
/// @tparam _Fp The function signature (e.g., void(), int(int), etc.)
template <size_t N, typename _FpReturnType, typename... _FpArgs>
class FatFunction<N, _FpReturnType(_FpArgs...)> {
  template <size_t, typename>
  friend class FatFunction;

 private:
  using _InvokerType = _FpReturnType (*)(std::byte *storage, _FpArgs... args);
  using _DeleterType = void (*)(std::byte *storage);
  using _MoverType = void (*)(std::byte *dest, std::byte *src);

 public:
  FatFunction() = default;
  FatFunction(std::nullptr_t) : FatFunction() {}

  /// @brief Constructs a FatFunction from a callable object.
  /// @tparam _Callable The type of the callable object
  /// @tparam (enable_if) Ensures that the callable fits within the allocated size N
  ///                    and is invocable with the specified signature.
  /// @param callable The callable object to store
  template <typename _Callable>
    requires CallableConcept<N, _Callable, _FpReturnType, _FpArgs...>
  FatFunction(_Callable &&callable) {
    using DecayedCallable = std::decay_t<_Callable>;
    new (storage_.data.data()) DecayedCallable(std::forward<_Callable>(callable));
    invoker_ = [](std::byte *storage, _FpArgs... args) -> _FpReturnType {
      DecayedCallable *callablePtr = reinterpret_cast<DecayedCallable *>(storage);
      return (*callablePtr)(std::forward<_FpArgs>(args)...);
    };
    if constexpr (std::is_trivially_destructible_v<DecayedCallable>) {
      // No custom deleter needed for trivially destructible types
      deleter_ = nullptr;
    } else {
      deleter_ = [](std::byte *storage) {
        DecayedCallable *callablePtr = reinterpret_cast<DecayedCallable *>(storage);
        callablePtr->~DecayedCallable();
      };
    }
    if constexpr (std::is_trivially_move_constructible_v<DecayedCallable>) {
      // No custom mover needed for trivially moveable types as memcpy is a fallback
      mover_ = nullptr;
    } else {
      mover_ = [](std::byte *dest, std::byte *src) {
        DecayedCallable *srcPtr = reinterpret_cast<DecayedCallable *>(src);
        new (dest) DecayedCallable(std::move(*srcPtr));
      };
    }
  }

  /// @brief Move constructor
  /// @param other The FatFunction to move from
  /// @tparam M The size of the callable in the other FatFunction, must be less than or equal to N to ensure it fits in the storage
  /// @note We can freely create FatFunction with bigger storage from FatFunction with smaller storage, but not the other way around
  FatFunction(FatFunction &&other) noexcept {
    if (other.invoker_) {
      if (other.mover_) {
        other.mover_(storage_.data.data(), other.storage_.data.data());
      } else {
        std::memcpy(storage_.data.data(), other.storage_.data.data(), N);
      }
      invoker_ = other.invoker_;
      deleter_ = other.deleter_;
      mover_ = other.mover_;
      other.reset();
    }
  }

  template <size_t M>
    requires(M < N)
  FatFunction(FatFunction<M, _FpReturnType(_FpArgs...)> &&other) {
    if (other.invoker_) {
      if (other.mover_) {
        other.mover_(storage_.data.data(), other.storage_.data.data());
      } else {
        std::memcpy(storage_.data.data(), other.storage_.data.data(), M);
      }
      invoker_ = other.invoker_;
      deleter_ = other.deleter_;
      mover_ = other.mover_;
      other.reset();
    }
  }

  /// @brief Move assignment operator
  /// @param other
  FatFunction &operator=(FatFunction &&other) noexcept {
    if (this != &other) {
      reset();
      if (other.invoker_) {
        if (other.mover_) {
          other.mover_(storage_.data.data(), other.storage_.data.data());
        } else {
          std::memcpy(storage_.data.data(), other.storage_.data.data(), N);
        }
        invoker_ = other.invoker_;
        deleter_ = other.deleter_;
        mover_ = other.mover_;
        other.reset();
      }
    }
    return *this;
  }

  /// @brief Call operator to invoke the stored callable
  /// @param ...args Arguments to pass to the callable
  /// @return The result of the callable invocation
  _FpReturnType operator()(_FpArgs &&...args) const {
    if (!invoker_) {
      throw std::bad_function_call();
    }
    return invoker_(storage_.data.data(), std::forward<_FpArgs>(args)...);
  }

  /// @brief Checks if the FatFunction contains a valid callable
  explicit operator bool() const noexcept {
    return invoker_ != nullptr;
  }

  /// @brief Destructor
  ~FatFunction() {
    reset();
  }

  /// @brief Releases the stored callable and returns its storage and deleter.
  /// @return A pair containing the storage array and the deleter function
  /// @note To clear resources properly after release, the user must call the deleter on the storage.
  std::pair<std::array<std::byte, N>, _DeleterType> release() {
    std::array<std::byte, N> storageCopy;
    std::memcpy(storageCopy.data(), storage_.data.data(), N);
    _DeleterType deleterCopy = deleter_;
    deleter_ = nullptr;
    invoker_ = nullptr;
    mover_ = nullptr;
    return {std::move(storageCopy), deleterCopy};
  }

 private:
  struct alignas(std::max_align_t) Storage {
    std::array<std::byte, N> data;
  };
  mutable Storage storage_;
  _InvokerType invoker_ = nullptr; // Function pointer to invoke the stored callable
  _DeleterType deleter_ = nullptr; // Function pointer to delete the stored callable
  _MoverType mover_ = nullptr;     // Function pointer to move the stored callable

  void reset() {
    if (deleter_) {
      deleter_(storage_.data.data());
    }
    deleter_ = nullptr;
    invoker_ = nullptr;
    mover_ = nullptr;
  }
};

} // namespace audioapi
