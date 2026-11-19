#pragma once
#include <cstddef>
#include <new>

#ifdef __APPLE__
#include <sys/mman.h>
#endif

template <typename T, std::size_t Align = 16>
class AlignedAllocator {
 public:
  using value_type = T;

  AlignedAllocator() noexcept = default;
  template <class U>
  explicit AlignedAllocator(const AlignedAllocator<U, Align> & /*other*/) noexcept {}

  T *allocate(std::size_t elementCount) {
    if (elementCount == 0) [[unlikely]] {
      return nullptr;
    }
    const std::size_t bytes = elementCount * sizeof(T);
#ifdef __APPLE__
    if (isMappedDirectly(bytes)) {
      void *mapping = mmap(nullptr, bytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
      if (mapping == MAP_FAILED) [[unlikely]] {
        throw std::bad_alloc();
      }
      return static_cast<T *>(mapping);
    }
#endif
    return static_cast<T *>(::operator new(bytes, std::align_val_t(Align)));
  }

  /// @param elementCount must equal the count given to the `allocate` call that returned
  /// @p block. It selects the path that releases the block, and unmapping needs the length.
  void deallocate(T *block, std::size_t elementCount) noexcept {
#ifdef __APPLE__
    const std::size_t bytes = elementCount * sizeof(T);
    if (isMappedDirectly(bytes)) {
      munmap(block, bytes);
      return;
    }
#endif
    ::operator delete(block, std::align_val_t(Align));
  }

  // Rebind allocator to type U (required by std::vector)
  template <class U>
  struct rebind {
    using other = AlignedAllocator<U, Align>;
  };

  // Comparison operators (required by std::vector)
  template <typename U, std::size_t UAlign>
  bool operator==(const AlignedAllocator<U, UAlign> & /*other*/) const noexcept {
    return Align == UAlign;
  }

  template <typename U, std::size_t UAlign>
  bool operator!=(const AlignedAllocator<U, UAlign> & /*other*/) const noexcept {
    return Align != UAlign;
  }

#ifdef __APPLE__

 private:
  // 4 MB, because large allocations get only fraction of speedup from the kernel memory cache
  static constexpr std::size_t DIRECT_MAPPING_THRESHOLD_BYTES = std::size_t{4} * 1024 * 1024;
  static constexpr std::size_t SMALLEST_PAGE_SIZE_BYTES = 4096;

  /// A mapping starts on a page boundary, which is the only alignment it guarantees.
  static_assert(
      Align <= SMALLEST_PAGE_SIZE_BYTES,
      "a directly mapped block cannot satisfy an alignment above the page size");

  /// Large blocks bypass malloc and are mapped from, and returned to, the kernel.
  /// Apple's malloc keeps a freed large block counted in the process footprint.
  /// A freed decoded buffer, and every block a growing buffer leaves behind,
  /// would stay charged to the app. Unmapping returns the pages immediately.
  static constexpr bool isMappedDirectly(std::size_t bytes) noexcept {
    return bytes >= DIRECT_MAPPING_THRESHOLD_BYTES;
  }
#endif
};
