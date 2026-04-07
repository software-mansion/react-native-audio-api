#include <audioapi/core/utils/graph/Graph.h>
#include <gtest/gtest.h>
#include <chrono>
#include <memory>
#include <thread>
#include <vector>
#include "AudioThreadGuard.h"
#include "MockGraphProcessor.h"
#include "TestGraphUtils.h"

using namespace audioapi::utils::graph;
using audioapi::test::AudioThreadGuard;
using audioapi::test::MockGraphProcessor;
using audioapi::utils::DisposerImpl;

// =========================================================================
// GraphNodeGrowthTest
//
// Verifies the allocation behaviour of sendNodeGrowIfNeeded().
//
// The method sends a lambda [newCap](...){ graph.reserveNodes(newCap); }
// through the SPSC event channel. That lambda is executed by the audio
// thread inside processEvents() → AudioGraph::reserveNodes() →
// std::vector::reserve(), which allocates heap memory ON the audio thread.
//
// The tests below expose this by running a MockGraphProcessor (audio thread
// instrumented with AudioThreadGuard) while the main thread adds nodes.
// =========================================================================

class GraphNodeGrowthTest : public ::testing::Test {
 protected:
  using PNode = ProcessableMockNode;
  using HNode = HostGraph::Node;

  static constexpr size_t kPayloadSize = audioapi::DISPOSER_PAYLOAD_SIZE;
  DisposerImpl<kPayloadSize> disposer_{64};
};

// ── Test 1: Node grow events must not allocate on the audio thread ─────────
//
// Creates a Graph WITHOUT pre-reserved node capacity (nodeCapacity_ == 0).
// Adding 1000 nodes triggers multiple sendNodeGrowIfNeeded() calls, each
// sending a grow event that the audio thread executes inside processEvents().
//
// Currently FAILS because the grow event lambda calls reserveNodes() on the
// audio thread → std::vector::reserve() → heap allocation.
// The fix: pre-allocate the buffer on the main thread (same pattern as
// sendPoolGrowIfNeeded) and only hand off the ready buffer via the event.
TEST_F(GraphNodeGrowthTest, NodeGrowEventsDoNotAllocateOnAudioThread) {
  if (!audioapi::test::AudioThreadGuard::isEnabled()) {
    GTEST_SKIP() << "AudioThreadGuard operator new/delete overrides are disabled "
                    "under ASan/TSan — allocation tracking unavailable";
  }

  // No initial capacity → nodeCapacity_ = 0, every threshold triggers a grow
  auto graph = std::make_unique<Graph>(4096, &disposer_);

  MockGraphProcessor<PNode> processor(*graph);
  processor.start();

  constexpr size_t kNodeCount = 1000;
  std::vector<HNode *> nodes;
  nodes.reserve(kNodeCount);

  for (size_t i = 0; i < kNodeCount; ++i) {
    nodes.push_back(graph->addNode(std::make_unique<PNode>()));

    // Periodic yield so the audio thread can process grow events
    // while the main thread is still adding nodes.
    if (i % 50 == 0) {
      std::this_thread::sleep_for(std::chrono::microseconds(500));
    }
  }

  // Drain all pending events
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  processor.stop();

  EXPECT_GT(processor.cyclesCompleted(), 0u)
      << "Audio thread should have completed at least one cycle";

  EXPECT_TRUE(processor.allocationClean())
      << "Audio thread allocated " << processor.allocationViolations() << " times across "
      << processor.cyclesCompleted()
      << " cycles — sendNodeGrowIfNeeded() must not allocate on the audio thread";
}

// ── Test 2: Pre-reserved capacity prevents audio-thread allocations ────────
//
// When the Graph is constructed with sufficient initial capacity
// (4-arg constructor), sendNodeGrowIfNeeded() never fires and the audio
// thread stays allocation-free — even for 1000 nodes.
// This serves as a control / regression baseline.
TEST_F(GraphNodeGrowthTest, PreReservedCapacityKeepsAudioThreadAllocationFree) {
  if (!audioapi::test::AudioThreadGuard::isEnabled()) {
    GTEST_SKIP() << "AudioThreadGuard operator new/delete overrides are disabled "
                    "under ASan/TSan — allocation tracking unavailable";
  }

  constexpr size_t kNodeCount = 1000;

  // Reserve enough upfront so sendNodeGrowIfNeeded() never fires
  const auto maxNodes = static_cast<std::uint32_t>(kNodeCount + 64);
  const std::uint32_t maxEdges = 64;
  auto graph = std::make_unique<Graph>(4096, &disposer_, maxNodes, maxEdges);

  MockGraphProcessor<PNode> processor(*graph);
  processor.start();

  std::vector<HNode *> nodes;
  nodes.reserve(kNodeCount);

  for (size_t i = 0; i < kNodeCount; ++i) {
    nodes.push_back(graph->addNode(std::make_unique<PNode>()));
    if (i % 50 == 0) {
      std::this_thread::sleep_for(std::chrono::microseconds(500));
    }
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  processor.stop();

  EXPECT_GT(processor.cyclesCompleted(), 0u)
      << "Audio thread should have completed at least one cycle";

  EXPECT_TRUE(processor.allocationClean())
      << "Audio thread allocated " << processor.allocationViolations() << " times across "
      << processor.cyclesCompleted() << " cycles despite pre-reserved capacity";
}
