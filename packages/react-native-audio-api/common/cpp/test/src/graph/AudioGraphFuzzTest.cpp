#include <audioapi/core/utils/graph/AudioGraph.hpp>
#include <audioapi/core/utils/graph/NodeHandle.hpp>
#include <gtest/gtest.h>
#include "TestGraphUtils.h"

#include <algorithm>
#include <iostream>
#include <memory>
#include <random>
#include <set>
#include <string>
#include <vector>

namespace audioapi::utils::graph {

/// Randomized single-threaded AudioGraph test.
/// Directly mutates the AudioGraph the same way HostGraph events do,
/// then calls process() and verifies invariants:
///   1. No input index >= size (dangling index)
///   2. No duplicate inputs
///   3. No cycles (Kahn's algorithm)
///   4. handle->index matches actual position
class AudioGraphFuzzTest : public ::testing::TestWithParam<uint64_t> {
 protected:
  using MNode = MockNode;

  AudioGraph<MNode> graph;
  std::mt19937_64 rng;

  // Track live handles so we can reference them
  std::vector<std::shared_ptr<NodeHandle<MNode>>> handles;
  size_t nextId = 0;

  void SetUp() override {
    rng.seed(GetParam());
  }

  // ── Helpers ─────────────────────────────────────────────────────────────

  std::shared_ptr<NodeHandle<MNode>> doAddNode() {
    auto h = std::make_shared<NodeHandle<MNode>>(0, std::make_unique<MNode>());
    graph.addNode(h);
    graph[h->index].test_node_identifier__ = nextId++;
    handles.push_back(h);
    return h;
  }

  void doOrphanNode(size_t handleIdx) {
    auto &h = handles[handleIdx];
    graph[h->index].orphaned = true;
    // Don't null the handle — AudioGraph still has the node.
    // pickLive() will skip it because it checks orphaned status.
  }

  void doAddEdge(std::shared_ptr<NodeHandle<MNode>> &from, std::shared_ptr<NodeHandle<MNode>> &to) {
    auto fromIdx = from->index;
    auto toIdx = to->index;
    // Verify at point-of-add that this edge doesn't create a duplicate
    for (auto inp : graph.pool().view(graph[toIdx].input_head)) {
      if (inp == fromIdx) {
        FAIL() << "BUG: doAddEdge called but edge already exists! "
               << "from=" << fromIdx << "(id=" << graph[fromIdx].test_node_identifier__
               << ") to=" << toIdx << "(id=" << graph[toIdx].test_node_identifier__ << ")";
      }
    }
    graph.pool().push(graph[toIdx].input_head, fromIdx);
    graph.markDirty();
  }

  void doRemoveEdge(
      std::shared_ptr<NodeHandle<MNode>> &from,
      std::shared_ptr<NodeHandle<MNode>> &to) {
    // Same as what HostGraph's removeEdge event does
    graph.pool().remove(graph[to->index].input_head, from->index);
    graph.markDirty();
  }

  // ── Invariant checks ───────────────────────────────────────────────────

  /// Check that no input index is out of bounds.
  void assertNoOOBInputs() {
    auto n = static_cast<uint32_t>(graph.size());
    for (uint32_t i = 0; i < n; i++) {
      for (auto inp : graph.pool().view(graph[i].input_head)) {
        ASSERT_LT(inp, n) << "Node[" << i << "] (id=" << graph[i].test_node_identifier__
                          << ") has OOB input index " << inp << " (graph size=" << n << ")";
      }
    }
  }

  /// Check that no node has duplicate inputs. Returns true if duplicates found.
  bool checkDuplicateInputs(const std::string &context) {
    bool found = false;
    auto n = static_cast<uint32_t>(graph.size());
    for (uint32_t i = 0; i < n; i++) {
      std::set<uint32_t> seen;
      for (auto inp : graph.pool().view(graph[i].input_head)) {
        if (!seen.insert(inp).second) {
          if (!found) {
            std::cerr << "\n!!! DUPLICATE INPUTS " << context << "\n";
            found = true;
          }
          std::cerr << "  Node[" << i << "] (id=" << graph[i].test_node_identifier__
                    << ") has duplicate input " << inp
                    << " (id=" << graph[inp].test_node_identifier__ << ")\n";
        }
      }
    }
    return found;
  }

  /// Check that handle->index matches actual position.
  void assertHandleIndicesCorrect() {
    auto n = static_cast<uint32_t>(graph.size());
    for (uint32_t i = 0; i < n; i++) {
      ASSERT_EQ(graph[i].handle->index, i)
          << "Node[" << i << "] (id=" << graph[i].test_node_identifier__
          << ") handle->index=" << graph[i].handle->index;
    }
  }

  /// Check that graph has no cycles via Kahn's algorithm.
  bool hasCycle() const {
    auto n = static_cast<uint32_t>(graph.size());
    if (n <= 1)
      return false;

    std::vector<uint32_t> outDeg(n, 0);
    for (uint32_t i = 0; i < n; i++) {
      for (auto inp : graph.pool().view(graph[i].input_head)) {
        if (inp < n)
          outDeg[inp]++;
      }
    }

    std::vector<uint32_t> queue;
    for (uint32_t i = 0; i < n; i++) {
      if (outDeg[i] == 0)
        queue.push_back(i);
    }

    uint32_t visited = 0;
    size_t head = 0;
    while (head < queue.size()) {
      auto idx = queue[head++];
      visited++;
      for (auto inp : graph.pool().view(graph[idx].input_head)) {
        if (inp < n && --outDeg[inp] == 0)
          queue.push_back(inp);
      }
    }
    return visited != n;
  }

  void dumpGraph() const {
    auto n = static_cast<uint32_t>(graph.size());
    std::cerr << "=== AudioGraph dump (n=" << n << ") ===\n";
    for (uint32_t i = 0; i < n; i++) {
      std::cerr << "  [" << i << "] id=" << graph[i].test_node_identifier__
                << " orphaned=" << graph[i].orphaned << " inputs={";
      bool first = true;
      for (auto inp : graph.pool().view(graph[i].input_head)) {
        if (!first)
          std::cerr << ",";
        first = false;
        if (inp < n)
          std::cerr << inp << "(id=" << graph[inp].test_node_identifier__ << ")";
        else
          std::cerr << inp << "(OOB!)";
      }
      std::cerr << "}\n";
    }
  }

  void assertAllInvariants(const std::string &context) {
    assertNoOOBInputs();
    if (checkDuplicateInputs(context)) {
      dumpGraph();
      FAIL() << "Duplicate inputs found " << context;
    }
    assertHandleIndicesCorrect();
    if (hasCycle()) {
      dumpGraph();
      FAIL() << "Cycle detected " << context;
    }
  }

  // ── Utility ─────────────────────────────────────────────────────────────

  /// Pick a random live (non-null, non-orphaned) handle index
  int pickLive() {
    std::vector<int> live;
    for (int i = 0; i < static_cast<int>(handles.size()); i++) {
      if (handles[i] && handles[i]->index < graph.size() && !graph[handles[i]->index].orphaned)
        live.push_back(i);
    }
    if (live.empty())
      return -1;
    return live[std::uniform_int_distribution<size_t>(0, live.size() - 1)(rng)];
  }

  /// Simple DFS cycle check: would adding from->to create a cycle?
  /// (Check if there's already a path from "from" to "to" via inputs,
  ///  i.e. "to" can reach "from" through the input edges.)
  bool wouldCreateCycle(uint32_t fromIdx, uint32_t toIdx) {
    // If adding fromIdx as input of toIdx, check if toIdx can reach fromIdx
    // through existing inputs (meaning fromIdx depends on toIdx already).
    auto n = static_cast<uint32_t>(graph.size());
    if (fromIdx == toIdx)
      return true;

    std::vector<bool> visited(n, false);
    std::vector<uint32_t> stack;
    stack.push_back(fromIdx);
    visited[fromIdx] = true;

    while (!stack.empty()) {
      auto cur = stack.back();
      stack.pop_back();
      for (auto inp : graph.pool().view(graph[cur].input_head)) {
        if (inp == toIdx)
          return true;
        if (inp < n && !visited[inp]) {
          visited[inp] = true;
          stack.push_back(inp);
        }
      }
    }
    return false;
  }

  /// Check if edge already exists
  bool edgeExists(uint32_t fromIdx, uint32_t toIdx) {
    for (auto inp : graph.pool().view(graph[toIdx].input_head)) {
      if (inp == fromIdx)
        return true;
    }
    return false;
  }
};

// =========================================================================
// Test: random operations directly on AudioGraph, verify after each process()
// =========================================================================

TEST_P(AudioGraphFuzzTest, RandomOps) {
  size_t initialCount = std::uniform_int_distribution<size_t>(8, 64)(rng);
  size_t opCount = std::uniform_int_distribution<size_t>(100, 1000)(rng);

  // Seed nodes
  for (size_t i = 0; i < initialCount; i++) {
    doAddNode();
  }
  graph.process();
  assertAllInvariants("after initial seeding");

  for (size_t i = 0; i < opCount; i++) {
    size_t op = std::uniform_int_distribution<size_t>(0, 99)(rng);

    if (op < 15) {
      // Add node
      doAddNode();

    } else if (op < 30) {
      // Orphan a random live node
      int idx = pickLive();
      if (idx >= 0) {
        doOrphanNode(idx);
      }

    } else if (op < 65) {
      // Add edge (only if it won't create a cycle or duplicate)
      int a = pickLive();
      int b = pickLive();
      if (a >= 0 && b >= 0 && a != b) {
        auto fromIdx = handles[a]->index;
        auto toIdx = handles[b]->index;
        if (!edgeExists(fromIdx, toIdx) && !wouldCreateCycle(fromIdx, toIdx)) {
          doAddEdge(handles[a], handles[b]);
        }
      }

    } else if (op < 80) {
      // Remove edge
      int b = pickLive();
      if (b >= 0) {
        auto toIdx = handles[b]->index;
        // Collect input values into a temporary vector for random selection
        std::vector<uint32_t> inputVals;
        for (auto inp : graph.pool().view(graph[toIdx].input_head)) {
          inputVals.push_back(inp);
        }
        if (!inputVals.empty()) {
          // Pick a random input to remove
          auto inputIdx =
              inputVals[std::uniform_int_distribution<size_t>(0, inputVals.size() - 1)(rng)];
          // Find the handle with this index
          for (auto &h : handles) {
            if (h && h->index == inputIdx) {
              doRemoveEdge(h, handles[b]);
              break;
            }
          }
        }
      }

    } else {
      // Process
      bool hadDupsBefore = checkDuplicateInputs("BEFORE process at op " + std::to_string(i));
      graph.process();

      // Null out handles for nodes that were compacted away.
      for (auto &h : handles) {
        if (h && (h->index >= graph.size() || graph[h->index].handle != h)) {
          h = nullptr;
        }
      }

      bool hasDupsAfter = checkDuplicateInputs("AFTER process at op " + std::to_string(i));
      if (!hadDupsBefore && hasDupsAfter) {
        std::cerr << "\n*** process() INTRODUCED duplicates at op " << i << " ***\n";
        dumpGraph();
        FAIL() << "process() introduced duplicate inputs at op " << i;
      }

      assertAllInvariants("after process() at op " + std::to_string(i));
    }

    // Also check OOB before process (should never have OOB inputs)
    // Note: after process we do full check above, but between ops we at least
    // verify no OOB indices crept in
    assertNoOOBInputs();
  }

  // Final process
  graph.process();
  for (auto &h : handles) {
    if (h && (h->index >= graph.size() || graph[h->index].handle != h)) {
      h = nullptr;
    }
  }
  assertAllInvariants("after final process()");
}

INSTANTIATE_TEST_SUITE_P(
    Seeds,
    AudioGraphFuzzTest,
    ::testing::Range(uint64_t{0}, uint64_t{100}),
    [](const ::testing::TestParamInfo<uint64_t> &info) {
      return "seed_" + std::to_string(info.param);
    });

} // namespace audioapi::utils::graph
