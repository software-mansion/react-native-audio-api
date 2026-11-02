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

TEST(ThreadPoolTest, WaitCoversEveryTaskOfEachBatch) {
  std::atomic<int> counter{0};
  ThreadPool<thread_pool::kSmallTaskStorageBytes> pool(4, 8, 8);

  constexpr int kBatches = 2000;
  constexpr int kTasksPerBatch = 3;
  for (int batch = 1; batch <= kBatches; ++batch) {
    for (int i = 0; i < kTasksPerBatch; ++i) {
      pool.schedule([&counter] { counter.fetch_add(1, std::memory_order_relaxed); });
    }
    pool.wait();
    ASSERT_EQ(counter.load(std::memory_order_relaxed), batch * kTasksPerBatch);
  }
}

TEST(ThreadPoolTest, TryScheduleRefusesWhenQueueIsFullAndWaitStillBalances) {
  std::atomic<int> counter{0};
  std::atomic<bool> gate{false};
  // 1 worker, 1-slot queues (an SPSC ring of size 2 holds one element): the worker takes one
  // task and blocks on the gate, the load balancer parks one more in the worker's queue,
  // after that the balancer queue fills.
  ThreadPool<thread_pool::kSmallTaskStorageBytes> pool(1, 2, 2);

  int accepted = 0;
  int refused = 0;
  for (int i = 0; i < 16; ++i) {
    bool ok = pool.trySchedule([&] {
      gate.wait(false);
      counter.fetch_add(1, std::memory_order_relaxed);
    });
    ok ? ++accepted : ++refused;
  }
  EXPECT_GT(refused, 0) << "a 1-slot pool cannot accept 16 blocked tasks";
  EXPECT_GT(accepted, 0);

  gate = true;
  gate.notify_all();
  pool.wait(); // must not hang: refused tasks were never counted as pending
  EXPECT_EQ(counter.load(std::memory_order_relaxed), accepted);
}

} // namespace audioapi::test
