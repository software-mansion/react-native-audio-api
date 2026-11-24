#include <audioapi/core/inputs/RecorderInputGate.h>
#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <thread>

using namespace audioapi;

// NOLINTBEGIN

TEST(RecorderInputGateTest, OnlyACallbackEnteringAnArmedGateDelivers) {
  int deliveredFrames = 0;
  RecorderInputGate gate([&deliveredFrames](const float *const *, int, int numFrames) {
    deliveredFrames += numFrames;
  });
  const auto deliverThroughPass = [&gate](int numFrames) {
    const auto pass = gate.enter();
    pass.deliver(nullptr, 1, numFrames);
    return static_cast<bool>(pass);
  };

  EXPECT_FALSE(deliverThroughPass(1));

  gate.arm();
  EXPECT_TRUE(deliverThroughPass(10));

  gate.disarmAndWait();
  EXPECT_FALSE(deliverThroughPass(100));
  EXPECT_EQ(deliveredFrames, 10);
}

// A platform callback converts into buffers a reopen reallocates before it delivers; the drain has
// to wait for that part too.
TEST(RecorderInputGateTest, DisarmWaitsForACallbackThatHasNotDeliveredYet) {
  RecorderInputGate gate(nullptr);
  gate.arm();
  std::atomic<bool> entered{false};
  std::atomic<bool> converted{false};
  std::atomic<bool> drained{false};

  std::thread audioThread([&] {
    const auto pass = gate.enter();
    EXPECT_TRUE(pass);
    entered = true;
    while (!converted) {
      std::this_thread::yield();
    }
  });
  while (!entered) {
    std::this_thread::yield();
  }

  std::thread controlThread([&] {
    gate.disarmAndWait();
    drained = true;
  });
  std::this_thread::sleep_for(std::chrono::milliseconds(20));
  EXPECT_FALSE(drained);

  converted = true;
  audioThread.join();
  controlThread.join();
  EXPECT_TRUE(drained);
}

// NOLINTEND
