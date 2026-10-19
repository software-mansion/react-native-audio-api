#include <audioapi/utils/PooledTaskExecutor.hpp>
#include <audioapi/utils/SerialTaskExecutor.hpp>
#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>

namespace audioapi::test {

namespace {

/// Blocks the test thread until @p expected tasks have reported in.
class CompletionLatch {
 public:
  void arrive() {
    std::scoped_lock lock(mutex_);
    ++arrived_;
    arrivedChanged_.notify_all();
  }

  void waitFor(int expected) {
    std::unique_lock lock(mutex_);
    arrivedChanged_.wait(lock, [&] { return arrived_ >= expected; });
  }

 private:
  std::mutex mutex_;
  std::condition_variable arrivedChanged_;
  int arrived_{0};
};

} // namespace

TEST(SerialTaskExecutorTest, RunsTasksInSchedulingOrder) {
  constexpr int kTaskCount = 64;
  std::vector<int> order;
  CompletionLatch latch;
  SerialTaskExecutor lane(8);

  for (int i = 0; i < kTaskCount; ++i) {
    lane.schedule([&order, &latch, i] {
      // Uneven task durations would expose any reordering if more than one thread ran them.
      if (i % 3 == 0) {
        std::this_thread::sleep_for(std::chrono::microseconds(50));
      }
      order.push_back(i);
      latch.arrive();
    });
  }
  latch.waitFor(kTaskCount);

  ASSERT_EQ(order.size(), static_cast<size_t>(kTaskCount));
  for (int i = 0; i < kTaskCount; ++i) {
    EXPECT_EQ(order[i], i);
  }
}

TEST(SerialTaskExecutorTest, DestructorDrainsQueuedTasks) {
  std::atomic<int> counter{0};
  {
    SerialTaskExecutor lane(8);
    for (int i = 0; i < 4; ++i) {
      lane.schedule([&counter] {
        std::this_thread::sleep_for(std::chrono::microseconds(100));
        counter.fetch_add(1, std::memory_order_relaxed);
      });
    }
  }

  EXPECT_EQ(counter.load(std::memory_order_relaxed), 4);
}

TEST(PooledTaskExecutorTest, RunsScheduledTasks) {
  constexpr int kTaskCount = 16;
  std::atomic<int> counter{0};
  CompletionLatch latch;
  PooledTaskExecutor pool(2, 8, 8);

  for (int i = 0; i < kTaskCount; ++i) {
    pool.schedule([&counter, &latch] {
      counter.fetch_add(1, std::memory_order_relaxed);
      latch.arrive();
    });
  }
  latch.waitFor(kTaskCount);

  EXPECT_EQ(counter.load(std::memory_order_relaxed), kTaskCount);
}

} // namespace audioapi::test
