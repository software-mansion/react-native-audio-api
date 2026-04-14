#pragma once
#include <cstddef>
#include <new>

template <typename T, std::size_t Align = 16>
class AlignedAllocator {
 public:
  using value_type = T;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;

  AlignedAllocator() noexcept = default;
  template <class U>
  AlignedAllocator(const AlignedAllocator<U, Align> &) noexcept {}

  T *allocate(std::size_t n) {
    // We want to maximize performance on hot paths, so we hint unlikely branches
    if (n == 0) [[unlikely]] {
      return nullptr;
    }
    std::size_t bytes = n * sizeof(T);
    // C++17 aligned new
    void *p = ::operator new(bytes, std::align_val_t(Align));

    // We have more serious problems if this happens than speed concerns
    // so we can opt the branch prediction
    if (!p) [[unlikely]] {
      throw std::bad_alloc();
    }
    return static_cast<T *>(p);
  }

  void deallocate(T *p, std::size_t) noexcept {
    ::operator delete(p, std::align_val_t(Align));
  }

  // Rebind allocator to type U (required by std::vector)
  template <class U>
  struct rebind {
    using other = AlignedAllocator<U, Align>;
  };

  // Comparison operators (required by std::vector)
  template <typename U, std::size_t UAlign>
  bool operator==(const AlignedAllocator<U, UAlign> &) const noexcept {
    return Align == UAlign;
  }

  template <typename U, std::size_t UAlign>
  bool operator!=(const AlignedAllocator<U, UAlign> &) const noexcept {
    return Align != UAlign;
  }
};
