#pragma once
#include <cstddef>
#include <new>

/// AddressSanitizer replaces malloc and free to poison the bytes around each block.
/// It sees nothing of a block mapped from the kernel, so sanitized builds stay on malloc.
#if defined(__SANITIZE_ADDRESS__)
#define RN_AUDIO_API_ADDRESS_SANITIZER 1
#elif defined(__has_feature)
#if __has_feature(address_sanitizer)
#define RN_AUDIO_API_ADDRESS_SANITIZER 1
#endif
#endif

#if defined(__APPLE__) && !defined(RN_AUDIO_API_ADDRESS_SANITIZER)
#define RN_AUDIO_API_DIRECT_MAPPING 1
#include <mach/vm_statistics.h>
#include <sys/mman.h>
#else
#define RN_AUDIO_API_DIRECT_MAPPING 0
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
#if RN_AUDIO_API_DIRECT_MAPPING
    if (isMappedDirectly(bytes)) {
      void *mapping = mmap(
          nullptr, bytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, DIRECT_MAPPING_VM_TAG, 0);
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
#if RN_AUDIO_API_DIRECT_MAPPING
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

#if RN_AUDIO_API_DIRECT_MAPPING
  static constexpr auto DIRECT_MAPPING_VM_TAG = VM_MAKE_TAG(VM_MEMORY_APPLICATION_SPECIFIC_1);
  // 4 MB, because large allocations get only fraction of speedup from the kernel memory cache
  static constexpr std::size_t DIRECT_MAPPING_THRESHOLD_BYTES = std::size_t{4} * 1024 * 1024;

 private:
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
