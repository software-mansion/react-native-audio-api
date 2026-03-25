#include <audioapi/core/utils/graph/Graph.hpp>
#include <gtest/gtest.h>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <memory>
#include <random>
#include <span>
#include <thread>
#include <utility>
#include <vector>
#include "AudioThreadGuard.h"
#include "MockGraphProcessor.h"
#include "TestGraphUtils.h"

using namespace audioapi::utils::graph;
using audioapi::test::MockGraphProcessor;

// =========================================================================
// Fixture — parameterized by seed for reproducible randomized testing
// =========================================================================

class GraphFuzzTest : public ::testing::TestWithParam<uint64_t> {
 protected:
  using PNode = ProcessableMockNode;
  using HNode = HostGraph<PNode>::Node;
  using Res = Graph<PNode>::Res;
  using ResultError = Graph<PNode>::ResultError;

  std::mt19937_64 rng;
  std::unique_ptr<Graph<PNode>> graph;
  std::vector<HNode *> nodes; // tracks live (non-removed) nodes
  size_t initialNodeCount;
  size_t operationCount;

  struct {
    size_t addNode = 0;
    size_t removeNode = 0;
    size_t addEdge = 0;
    size_t removeEdge = 0;
  } chances;

  void SetUp() override {
    rng.seed(GetParam());

    initialNodeCount = std::uniform_int_distribution<size_t>(16, 128)(rng);
    operationCount = std::uniform_int_distribution<size_t>(200, 2000)(rng);

    // Ensure graph growth does not happen on the audio thread during this fuzz run.
    const auto maxNodes = static_cast<std::uint32_t>(initialNodeCount + operationCount + 64);
    const auto maxEdges = static_cast<std::uint32_t>(operationCount * 2 + 64);
    graph = std::make_unique<Graph<PNode>>(4096, maxNodes, maxEdges);

    // Randomly partition the range 0..99 into 4 operation weights
    size_t total = 100;
    chances.addNode = std::uniform_int_distribution<size_t>(0, total)(rng);
    chances.removeNode = std::uniform_int_distribution<size_t>(0, total - chances.addNode)(rng);
    chances.addEdge =
        std::uniform_int_distribution<size_t>(0, total - chances.addNode - chances.removeNode)(rng);
    chances.removeEdge = total - chances.addNode - chances.removeNode - chances.addEdge;
  }

  enum class Operation { AddNode, RemoveNode, AddEdge, RemoveEdge, None };

  // ── Helpers ─────────────────────────────────────────────────────────────

  /// @brief Creates a ProcessableMockNode that busy-waits for a short
  /// duration (10–50 µs) simulating lightweight DSP work, then returns
  /// the sum of its inputs (or its initial value if no inputs).
  std::unique_ptr<PNode> makeBusyNode(int initialValue = 0) {
    // Each node gets a random busy-wait duration in [10, 50] µs
    auto busyNs = std::uniform_int_distribution<int>(10'000, 50'000)(rng);
    return std::make_unique<PNode>(
        [busyNs](std::span<const int> inputs) -> int {
          // Busy-wait (no syscalls, no allocation)
          auto start = std::chrono::steady_clock::now();
          auto target = std::chrono::nanoseconds(busyNs);
          while (std::chrono::steady_clock::now() - start < target) {
            // spin
          }
          // Sum inputs
          int sum = 0;
          for (int v : inputs)
            sum += v;
          return sum;
        },
        initialValue);
  }

  /// @brief Pick two distinct random nodes from the live set.
  std::pair<HNode *, HNode *> pickTwoNodes() {
    if (nodes.size() < 2)
      return {nullptr, nullptr};
    auto dist = std::uniform_int_distribution<size_t>(0, nodes.size() - 1);
    size_t a = dist(rng);
    size_t b = dist(rng);
    if (a == b)
      return {nullptr, nullptr};
    return {nodes[a], nodes[b]};
  }

  /// @brief Seeds the graph with `initialNodeCount` busy nodes.
  void seedGraph() {
    for (size_t i = 0; i < initialNodeCount; ++i) {
      auto *n = graph->addNode(makeBusyNode(static_cast<int>(i)));
      nodes.push_back(n);
    }
  }

  /// @brief Performs a single random graph mutation using only the public API.
  /// @return (operation, node) — node is present for add/remove, nullptr otherwise
  std::pair<Operation, HNode *> performRandomOperation() {
    size_t op = std::uniform_int_distribution<size_t>(0, 99)(rng);

    if (op < chances.addNode) {
      auto *n = graph->addNode(makeBusyNode());
      nodes.push_back(n);
      return {Operation::AddNode, n};
    }

    if (op < chances.addNode + chances.removeNode) {
      if (!nodes.empty()) {
        size_t idx = std::uniform_int_distribution<size_t>(0, nodes.size() - 1)(rng);
        HNode *target = nodes[idx];
        nodes.erase(nodes.begin() + static_cast<std::ptrdiff_t>(idx));
        (void)graph->removeNode(target);
        return {Operation::RemoveNode, target};
      }
      return {Operation::None, nullptr};
    }

    if (op < chances.addNode + chances.removeNode + chances.addEdge) {
      auto [from, to] = pickTwoNodes();
      if (from && to) {
        (void)graph->addEdge(from, to);
        return {Operation::AddEdge, nullptr};
      }
      return {Operation::None, nullptr};
    }

    // removeEdge
    {
      auto [from, to] = pickTwoNodes();
      if (from && to) {
        (void)graph->removeEdge(from, to);
        return {Operation::RemoveEdge, nullptr};
      }
    }
    return {Operation::None, nullptr};
  }
};

// =========================================================================
// Test: Audio thread processes while host thread mutates the graph
// =========================================================================

TEST_P(GraphFuzzTest, AudioThreadDoesNotAllocate) {
  seedGraph();

  // Start the mock audio processor on its own thread
  MockGraphProcessor<PNode> processor(*graph);
  processor.start();

  // Host thread: perform random mutations with small intervals
  for (size_t i = 0; i < operationCount; ++i) {
    performRandomOperation();
    // Small sleep to spread mutations over time (50–200 µs)
    std::this_thread::sleep_for(
        std::chrono::microseconds(std::uniform_int_distribution<int>(50, 200)(rng)));
  }

  // Let the audio thread drain remaining work
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  processor.stop();

  // ── Assertions ──────────────────────────────────────────────────────────

  EXPECT_GT(processor.cyclesCompleted(), 0u)
      << "Audio thread should have processed at least one cycle";

  EXPECT_TRUE(processor.allocationClean())
      << "Audio thread allocated/deallocated " << processor.allocationViolations()
      << " times across " << processor.cyclesCompleted() << " cycles";
}

TEST_P(GraphFuzzTest, GraphRemainsConsistentUnderConcurrency) {
  seedGraph();

  MockGraphProcessor<PNode> processor(*graph);
  processor.start();

  // Host thread: heavy burst of mutations
  for (size_t i = 0; i < operationCount; ++i) {
    performRandomOperation();
    // Occasional yield to let audio thread run
    if (i % 50 == 0) {
      std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  processor.stop();

  // If we got here without crash/hang/TSan report, the graph is consistent.
  EXPECT_GT(processor.cyclesCompleted(), 0u);
}

TEST_P(GraphFuzzTest, RapidAddRemoveCycles) {
  // Stress test: rapidly add and immediately remove nodes
  MockGraphProcessor<PNode> processor(*graph);
  processor.start();

  for (size_t i = 0; i < operationCount; ++i) {
    auto *n = graph->addNode(makeBusyNode(static_cast<int>(i)));
    // Immediately remove ~50% of them
    if (std::uniform_int_distribution<int>(0, 1)(rng) == 0) {
      (void)graph->removeNode(n);
    } else {
      nodes.push_back(n);
    }

    if (i % 100 == 0) {
      std::this_thread::sleep_for(std::chrono::microseconds(50));
    }
  }

  // Now remove everything that's still live
  for (auto *n : nodes) {
    (void)graph->removeNode(n);
  }
  nodes.clear();

  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  processor.stop();

  EXPECT_GT(processor.cyclesCompleted(), 0u);
  EXPECT_TRUE(processor.allocationClean())
      << "Audio thread had " << processor.allocationViolations() << " allocation violations";
}

// =========================================================================
// Instantiation: 100 seeds (0..99), each producing a unique random scenario
// =========================================================================

INSTANTIATE_TEST_SUITE_P(
    Seeds,
    GraphFuzzTest,
    ::testing::Range(uint64_t{0}, uint64_t{100}),
    [](const ::testing::TestParamInfo<uint64_t> &info) {
      return "seed_" + std::to_string(info.param);
    });
