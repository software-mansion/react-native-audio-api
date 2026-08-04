#include <audioapi/utils/ThreadPool.hpp>
#include <gtest/gtest.h>

#include <atomic>

namespace audioapi::test {

TEST(ThreadPoolTest, ScheduleRunsTaskAndWaitReturns) {
  std::atomic<int> counter{0};
  ThreadPool<thread_pool::kSmallTaskStorageBytes> pool(2, 8, 8);

  pool.schedule([&counter]() { counter.fetch_add(1, std::memory_order_relaxed); });
  pool.wait();

  EXPECT_EQ(counter.load(std::memory_order_relaxed), 1);
}

} // namespace audioapi::test
