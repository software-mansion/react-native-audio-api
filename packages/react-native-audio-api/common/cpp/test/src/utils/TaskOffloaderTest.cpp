#include <audioapi/utils/SpscChannel.hpp>
#include <audioapi/utils/TaskOffloader.hpp>
#include <gtest/gtest.h>
#include <atomic>
#include <chrono>
#include <limits>
#include <memory>
#include <thread>
#include <vector>

namespace audioapi::test {

struct TestWriterData {
  size_t slot = std::numeric_limits<size_t>::max();
  int numFrames = 0;
};

TEST(TaskOffloaderTest, ShutdownDrainsQueuedTasks) {
  std::atomic<int> processedCount{0};
  std::vector<int> processedFrames;
  processedFrames.reserve(8);

  task_offloader::TaskOffloader<
      TestWriterData,
      channels::spsc::OverflowStrategy::WAIT_ON_FULL,
      channels::spsc::WaitStrategy::YIELD>
      offloader(8, [&](TestWriterData data) {
        if (data.slot == std::numeric_limits<size_t>::max()) {
          return;
        }
        processedFrames.push_back(data.numFrames);
        processedCount.fetch_add(1, std::memory_order_relaxed);
      });

  for (int i = 0; i < 5; ++i) {
    offloader.getSender()->send(TestWriterData{.slot = static_cast<size_t>(i), .numFrames = i + 1});
  }

  offloader.shutdown();

  EXPECT_EQ(processedCount.load(), 5);
  ASSERT_EQ(processedFrames.size(), 5U);
  for (int i = 0; i < 5; ++i) {
    EXPECT_EQ(processedFrames[static_cast<size_t>(i)], i + 1);
  }
}

TEST(TaskOffloaderTest, DestructorDrainsQueuedTasks) {
  std::atomic<int> processedCount{0};

  {
    auto offloader = std::make_unique<task_offloader::TaskOffloader<
        TestWriterData,
        channels::spsc::OverflowStrategy::WAIT_ON_FULL,
        channels::spsc::WaitStrategy::YIELD>>(4, [&](TestWriterData data) {
      if (data.slot == std::numeric_limits<size_t>::max()) {
        return;
      }
      processedCount.fetch_add(1, std::memory_order_relaxed);
      std::this_thread::sleep_for(std::chrono::milliseconds(5));
    });

    for (int i = 0; i < 3; ++i) {
      offloader->getSender()->send(
          TestWriterData{.slot = static_cast<size_t>(i), .numFrames = 128});
    }
  }

  EXPECT_EQ(processedCount.load(), 3);
}

} // namespace audioapi::test
