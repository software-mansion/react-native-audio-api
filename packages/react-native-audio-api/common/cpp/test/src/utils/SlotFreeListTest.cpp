#include <audioapi/utils/SlotFreeList.hpp>
#include <gtest/gtest.h>
#include <limits>

namespace audioapi::test {

TEST(SlotFreeListTest, SeedAcquireRelease) {
  slots::SlotFreeList<4> freeList;
  freeList.seed();

  auto slot0 = freeList.tryAcquire();
  auto slot1 = freeList.tryAcquire();
  auto slot2 = freeList.tryAcquire();
  auto slot3 = freeList.tryAcquire();
  auto slot4 = freeList.tryAcquire();

  ASSERT_TRUE(slot0.has_value());
  ASSERT_TRUE(slot1.has_value());
  ASSERT_TRUE(slot2.has_value());
  ASSERT_TRUE(slot3.has_value());
  ASSERT_FALSE(slot4.has_value());

  freeList.release(slot1.value());
  auto recycled = freeList.tryAcquire();
  ASSERT_TRUE(recycled.has_value());
  EXPECT_EQ(recycled.value(), slot1.value());
}

TEST(SlotFreeListTest, DoubleReleaseDoesNotDuplicateSlot) {
  slots::SlotFreeList<4> freeList;
  freeList.seed();

  for (size_t i = 0; i < 4; ++i) {
    ASSERT_TRUE(freeList.tryAcquire().has_value());
  }

  auto slot = size_t{2};
  freeList.release(slot);
  freeList.release(slot); // idempotent: must not make the slot available twice

  ASSERT_TRUE(freeList.tryAcquire().has_value());
  EXPECT_FALSE(freeList.tryAcquire().has_value());
}

TEST(SlotFreeListTest, OutOfRangeReleaseIsIgnored) {
  slots::SlotFreeList<4> freeList;
  freeList.seed();

  for (size_t i = 0; i < 4; ++i) {
    ASSERT_TRUE(freeList.tryAcquire().has_value());
  }

  freeList.release(4);
  freeList.release(slots::SlotFreeList<4>::kSentinel);

  EXPECT_FALSE(freeList.tryAcquire().has_value());
}

TEST(SlotFreeListTest, SentinelValueIsOutOfRange) {
  slots::SlotFreeList<8> freeList;
  freeList.seed();

  EXPECT_EQ(slots::SlotFreeList<8>::kSentinel, std::numeric_limits<size_t>::max());

  for (size_t i = 0; i < 8; ++i) {
    auto slot = freeList.tryAcquire();
    ASSERT_TRUE(slot.has_value());
    EXPECT_NE(slot.value(), slots::SlotFreeList<8>::kSentinel);
  }
}

} // namespace audioapi::test
