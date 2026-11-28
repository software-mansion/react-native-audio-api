#include <ReactCommon/CallInvoker.h>
#include <audioapi/core/OfflineAudioContext.h>
#include <audioapi/events/AudioEventHandlerRegistry.h>
#include <audioapi/events/AudioEventProducer.h>
#include <gtest/gtest.h>
#include <latch>
#include <memory>
#include <thread>
#include <vector>

using namespace audioapi;

namespace {

/// Drops every hop to the JS thread: these tests only care whether the queue accepts events.
class DiscardingCallInvoker : public facebook::react::CallInvoker {
 public:
  void invokeAsync(facebook::react::CallFunc && /*func*/) noexcept override {}
  void invokeSync(facebook::react::CallFunc && /*func*/) override {}
};

// More than the queue's preallocated block count, so a leak of one block per producer or
// per thread runs the supply out within the loop.
constexpr int kMoreThanEveryPreallocatedBlock = 300;

// More producers than the queue's 8 preallocated blocks, so a producer that keeps its block
// after release() runs the supply out; fewer events than its 256 slots, so the test does not
// depend on the dispatch worker draining any of them.
constexpr int kMoreProducersThanPreallocatedBlocks = 64;

class AudioEventHandlerRegistryTest : public ::testing::Test {
 protected:
  // The registry refuses to dispatch without a runtime, but only dereferences it to call a
  // registered handler, and these tests register none and never reach the JS thread.
  facebook::jsi::Runtime *const unusedRuntime_ =
      reinterpret_cast<facebook::jsi::Runtime *>(alignof(facebook::jsi::Runtime));
  std::shared_ptr<AudioEventHandlerRegistry> registry_ =
      std::make_shared<AudioEventHandlerRegistry>(
          unusedRuntime_,
          std::make_shared<DiscardingCallInvoker>());

  bool dispatchFromAudioThread(AudioEventProducer &producer) {
    return registry_->dispatchEventFromAudioThread(
        producer, AudioEvent::ENDED, kBroadcastListenerId, EmptyPayload{});
  }
};

} // namespace

TEST_F(AudioEventHandlerRegistryTest, ShortLivedThreadsDoNotStarveAudioThreadProducers) {
  // Alive at once, like a GCD pool or per-context lane threads: a thread joined before the
  // next one starts hands its id, and with it its implicit producer, to its successor.
  std::latch allDispatched(kMoreThanEveryPreallocatedBlock);
  std::vector<std::thread> threads;
  threads.reserve(kMoreThanEveryPreallocatedBlock);
  for (int i = 0; i < kMoreThanEveryPreallocatedBlock; ++i) {
    threads.emplace_back([this, &allDispatched] {
      registry_->dispatchEvent(AudioEvent::STATE_CHANGE, kBroadcastListenerId, EmptyPayload{});
      allDispatched.arrive_and_wait();
    });
  }
  for (auto &thread : threads) {
    thread.join();
  }

  auto producer = registry_->createAudioEventProducer();
  EXPECT_TRUE(dispatchFromAudioThread(*producer));
}

TEST_F(AudioEventHandlerRegistryTest, ReleasedProducerHandsItsBlocksToTheNextOne) {
  // Kept alive the way a closed context's nodes keep its producer alive.
  std::vector<std::shared_ptr<AudioEventProducer>> closedContextProducers;

  for (int i = 0; i < kMoreProducersThanPreallocatedBlocks; ++i) {
    auto producer = registry_->createAudioEventProducer();
    ASSERT_TRUE(dispatchFromAudioThread(*producer)) << "context " << i;
    producer->release();
    closedContextProducers.push_back(producer);
  }
}

TEST_F(AudioEventHandlerRegistryTest, ReleasedProducerRefusesEvents) {
  auto producer = registry_->createAudioEventProducer();
  producer->release();

  EXPECT_FALSE(dispatchFromAudioThread(*producer));
}

TEST_F(AudioEventHandlerRegistryTest, DestroyedContextReleasesItsProducer) {
  auto context = std::make_shared<OfflineAudioContext>(1, 128, 44100.0F, registry_);
  // Held the way a node outliving its context holds it.
  auto producer = context->getAudioEventProducer();
  context.reset();

  EXPECT_FALSE(dispatchFromAudioThread(*producer));
}
