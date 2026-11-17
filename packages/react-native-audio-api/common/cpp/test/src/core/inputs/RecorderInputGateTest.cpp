#include <audioapi/core/inputs/RecorderInputGate.h>
#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <thread>

using namespace audioapi;

// NOLINTBEGIN

// No test delivers frames, so the gate never reaches a recorder.

TEST(RecorderInputGateTest, OnlyACallbackEnteringAnArmedGateIsAdmitted) {
  RecorderInputGate gate(nullptr);
  EXPECT_FALSE(gate.enter());

  gate.arm();
  EXPECT_TRUE(gate.enter());

  gate.disarmAndWait();
  EXPECT_FALSE(gate.enter());
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
