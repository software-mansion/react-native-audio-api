#include <audioapi/utils/AlignedAllocator.hpp>
#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>
#include <vector>

#if RN_AUDIO_API_DIRECT_MAPPING
#include <mach/mach.h>
#include <mach/mach_vm.h>
#endif

#if defined(__has_feature)
#if __has_feature(address_sanitizer)
#define RN_AUDIO_API_TEST_ASAN 1
#endif
#endif
#if defined(__SANITIZE_ADDRESS__)
#define RN_AUDIO_API_TEST_ASAN 1
#endif

namespace audioapi::test {

namespace {

constexpr std::size_t LARGE_BLOCK_BYTES = std::size_t{8} * 1024 * 1024;

bool isAlignedTo(const void *pointer, std::size_t alignment) {
  return reinterpret_cast<std::uintptr_t>(pointer) % alignment == 0;
}

#if RN_AUDIO_API_DIRECT_MAPPING
/// Reads the VM tag of the region that contains @p address, or -1 when no region does.
int vmTagOfRegionContaining(const void *address) {
  auto regionStart = static_cast<mach_vm_address_t>(reinterpret_cast<std::uintptr_t>(address));
  mach_vm_size_t regionSize = 0;
  natural_t depth = 0;
  vm_region_submap_info_data_64_t info{};
  mach_msg_type_number_t infoCount = VM_REGION_SUBMAP_INFO_COUNT_64;
  const kern_return_t result = mach_vm_region_recurse(
      mach_task_self(),
      &regionStart,
      &regionSize,
      &depth,
      reinterpret_cast<vm_region_recurse_info_t>(&info),
      &infoCount);
  const auto requested = reinterpret_cast<std::uintptr_t>(address);
  if (result != KERN_SUCCESS || regionStart > requested) {
    return -1;
  }
  return static_cast<int>(info.user_tag);
}
#endif

} // namespace

TEST(AlignedAllocatorTest, ZeroCountAllocatesNothing) {
  AlignedAllocator<float> allocator;
  float *block = allocator.allocate(0);
  EXPECT_EQ(block, nullptr);
  allocator.deallocate(block, 0);
}

TEST(AlignedAllocatorTest, SmallBlocksHonourTheRequestedAlignment) {
  AlignedAllocator<float, 64> allocator;
  for (std::size_t count : {std::size_t{1}, std::size_t{3}, std::size_t{128}, std::size_t{4097}}) {
    float *block = allocator.allocate(count);
    ASSERT_NE(block, nullptr);
    EXPECT_TRUE(isAlignedTo(block, 64)) << "count " << count;
    std::memset(block, 0, count * sizeof(float));
    allocator.deallocate(block, count);
  }
}

TEST(AlignedAllocatorTest, LargeBlockIsAlignedAndHoldsItsContents) {
  AlignedAllocator<float, 64> allocator;
  const std::size_t count = LARGE_BLOCK_BYTES / sizeof(float);
  float *block = allocator.allocate(count);
  ASSERT_NE(block, nullptr);
  EXPECT_TRUE(isAlignedTo(block, 64));

  for (std::size_t i = 0; i < count; ++i) {
    block[i] = static_cast<float>(i);
  }
  for (std::size_t i = 0; i < count; i += 4099) {
    ASSERT_EQ(block[i], static_cast<float>(i));
  }
  EXPECT_EQ(block[count - 1], static_cast<float>(count - 1));
  allocator.deallocate(block, count);
}

TEST(AlignedAllocatorTest, VectorGrowingPastTheLargeBlockSizeKeepsItsContents) {
  std::vector<std::uint32_t, AlignedAllocator<std::uint32_t>> values;
  const std::size_t count = 2 * LARGE_BLOCK_BYTES / sizeof(std::uint32_t);
  for (std::uint32_t i = 0; i < count; ++i) {
    values.push_back(i);
  }
  ASSERT_EQ(values.size(), count);
  for (std::uint32_t i = 0; i < count; ++i) {
    ASSERT_EQ(values[i], i);
  }

  values.resize(16);
  values.shrink_to_fit();
  EXPECT_EQ(values.back(), 15u);
}

TEST(AlignedAllocatorTest, AllocatorsCompareEqualOnlyWithTheSameAlignment) {
  constexpr AlignedAllocator<float, 16> floats16;
  constexpr AlignedAllocator<int, 16> ints16;
  constexpr AlignedAllocator<float, 32> floats32;

  EXPECT_TRUE(floats16 == ints16);
  EXPECT_FALSE(floats16 != ints16);
  EXPECT_FALSE(floats16 == floats32);
  EXPECT_TRUE(floats16 != floats32);

  using Rebound = AlignedAllocator<float, 32>::rebind<double>::other;
  static_assert(std::is_same_v<Rebound, AlignedAllocator<double, 32>>);
  const Rebound rebound(floats32);
  EXPECT_TRUE(rebound == floats32);
}

#if RN_AUDIO_API_DIRECT_MAPPING

TEST(AlignedAllocatorTest, BlockAtTheThresholdIsATaggedMappingReturnedOnDeallocate) {
  using Allocator = AlignedAllocator<char>;
  Allocator allocator;
  const std::size_t count = Allocator::DIRECT_MAPPING_THRESHOLD_BYTES;
  char *block = allocator.allocate(count);
  ASSERT_NE(block, nullptr);
  EXPECT_TRUE(isAlignedTo(block, vm_page_size));
  EXPECT_EQ(vmTagOfRegionContaining(block), VM_MEMORY_APPLICATION_SPECIFIC_1);

  allocator.deallocate(block, count);
  EXPECT_NE(vmTagOfRegionContaining(block), VM_MEMORY_APPLICATION_SPECIFIC_1);
}

TEST(AlignedAllocatorTest, BlockBelowTheThresholdStaysOnMalloc) {
  using Allocator = AlignedAllocator<char>;
  Allocator allocator;
  const std::size_t count = Allocator::DIRECT_MAPPING_THRESHOLD_BYTES - 1;
  char *block = allocator.allocate(count);
  ASSERT_NE(block, nullptr);
  EXPECT_NE(vmTagOfRegionContaining(block), VM_MEMORY_APPLICATION_SPECIFIC_1);
  allocator.deallocate(block, count);
}

#endif

#ifdef RN_AUDIO_API_TEST_ASAN

/// A mapped block is invisible to AddressSanitizer, so the large path must stay on malloc.
TEST(AlignedAllocatorDeathTest, AddressSanitizerCatchesOverflowOfALargeBlock) {
  AlignedAllocator<char> allocator;
  EXPECT_DEATH(
      {
        char *block = allocator.allocate(LARGE_BLOCK_BYTES);
        volatile char *overflowing = block + LARGE_BLOCK_BYTES;
        *overflowing = 1;
        allocator.deallocate(block, LARGE_BLOCK_BYTES);
      },
      "heap-buffer-overflow");
}

#endif

} // namespace audioapi::test
