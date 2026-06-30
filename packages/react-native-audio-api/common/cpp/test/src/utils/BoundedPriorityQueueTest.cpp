// Exercises the non-pmr fallback path of BoundedPriorityQueue (the std::multiset
// + FixedBlockPool implementation used on runtimes whose libc++ has no std::pmr,
// e.g. iOS < 17). AUDIOAPI_HAS_PMR is forced to 0 so this path is covered even on
// hosts whose libc++ does have pmr; the default (std::pmr) path is exercised by
// the rest of the suite, which compiles the library with the auto-detected gate.
#define AUDIOAPI_HAS_PMR 0

#include <audioapi/utils/BoundedPriorityQueue.hpp>
#include <gtest/gtest.h>

#include <cstdint>
#include <utility>
#include <vector>

using namespace audioapi;

namespace {

struct Event {
  int64_t time;
  int id;
};

// Transparent comparator (heterogeneous key lookups), mirroring how the audio
// param queues use lowerBound/upperBound with a raw time key.
struct ByTime {
  using is_transparent = void;
  bool operator()(const Event &a, const Event &b) const {
    return a.time < b.time;
  }
  bool operator()(const Event &a, int64_t b) const {
    return a.time < b;
  }
  bool operator()(int64_t a, const Event &b) const {
    return a < b.time;
  }
};

} // namespace

TEST(BoundedPriorityQueueTest, KeepsAscendingOrderAndRespectsBounds) {
  BoundedPriorityQueue<Event, 8, ByTime> q;
  EXPECT_TRUE(q.isEmpty());

  // Insert with descending keys; the queue must keep them ascending.
  for (int i = 0; i < 8; ++i) {
    EXPECT_TRUE(q.push(Event{static_cast<int64_t>(100 - i * 10), i}));
  }
  EXPECT_TRUE(q.isFull());
  EXPECT_EQ(q.size(), 8u);
  EXPECT_FALSE(q.push(Event{5, 99})); // rejected when full
  EXPECT_EQ(q.size(), 8u);

  EXPECT_EQ(q.peekFront().time, 30); // 100 - 7*10
  EXPECT_EQ(q.peekBack().time, 100);

  int64_t prev = -1;
  for (auto it = q.begin(); it != q.end(); ++it) {
    EXPECT_LT(prev, it->time);
    prev = it->time;
  }
}

TEST(BoundedPriorityQueueTest, PopReturnsSmallestFirst) {
  BoundedPriorityQueue<Event, 4, ByTime> q;
  q.push(Event{30, 0});
  q.push(Event{10, 1});
  q.push(Event{20, 2});

  Event out{};
  EXPECT_TRUE(q.pop(out));
  EXPECT_EQ(out.time, 10);
  EXPECT_TRUE(q.pop(out));
  EXPECT_EQ(out.time, 20);
  EXPECT_TRUE(q.pop()); // drop 30 without retrieving
  EXPECT_TRUE(q.isEmpty());
  EXPECT_FALSE(q.pop(out)); // empty
}

TEST(BoundedPriorityQueueTest, RecyclesNodesUnderChurn) {
  // Far more insert/erase cycles than Capacity. If freed nodes were not
  // recycled the fixed pool would exhaust and allocate() would throw.
  BoundedPriorityQueue<Event, 8, ByTime> q;
  for (int i = 0; i < 6; ++i) {
    q.push(Event{static_cast<int64_t>(i), i});
  }
  int64_t key = 1000;
  for (int round = 0; round < 100000; ++round) {
    ASSERT_TRUE(q.push(Event{key++, round}));
    ASSERT_TRUE(q.pop());
  }
  EXPECT_EQ(q.size(), 6u);
}

TEST(BoundedPriorityQueueTest, BoundsExtractMutateAndReinsert) {
  BoundedPriorityQueue<Event, 8, ByTime> q;
  for (int i = 0; i < 5; ++i) {
    q.push(Event{static_cast<int64_t>(i * 10), i}); // 0,10,20,30,40
  }

  auto it = q.upperBound(static_cast<int64_t>(15)); // first key > 15 -> 20
  ASSERT_NE(it, q.end());
  EXPECT_EQ(it->time, 20);

  auto node = q.extract(it); // detach node holding 20
  node.value().time = 25;    // mutate key while detached (no reallocation)
  q.insert(q.upperBound(static_cast<int64_t>(25)), std::move(node));
  EXPECT_EQ(q.size(), 5u);

  std::vector<int64_t> times;
  for (auto i = q.begin(); i != q.end(); ++i) {
    times.push_back(i->time);
  }
  EXPECT_EQ(times, (std::vector<int64_t>{0, 10, 25, 30, 40}));

  // Range erase: drop everything >= 30.
  q.erase(q.lowerBound(static_cast<int64_t>(30)), q.end());
  EXPECT_EQ(q.size(), 3u);
  EXPECT_EQ(q.peekBack().time, 25);
}
