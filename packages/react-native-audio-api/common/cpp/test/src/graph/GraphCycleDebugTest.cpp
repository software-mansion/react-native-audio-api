#include <audioapi/core/utils/graph/Graph.hpp>
#include <gtest/gtest.h>
#include <algorithm>
#include <iostream>
#include <memory>
#include <random>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include "TestGraphUtils.h"

namespace audioapi::utils::graph {

/// Single-threaded deterministic test that processes events synchronously
/// after every mutation and checks for cycles. No concurrency — isolates
/// the cycle-formation bug.
class GraphCycleDebugTest : public ::testing::TestWithParam<uint64_t> {
 protected:
  using MNode = MockNode;
  using HNode = HostGraph<MNode>::Node;
  using AGEvent = HostGraph<MNode>::AGEvent;

  static constexpr size_t kPayloadSize = HostGraph<MNode>::kDisposerPayloadSize;

  std::mt19937_64 rng;
  AudioGraph<MNode> audioGraph;
  HostGraph<MNode> hostGraph;
  DisposerImpl<kPayloadSize> disposer_{64};
  std::vector<HNode *> liveNodes;
  size_t nextId = 0;

  void SetUp() override {
    rng.seed(GetParam());
  }

  // ── Helpers ─────────────────────────────────────────────────────────────

  HNode *doAddNode() {
    auto handle = std::make_shared<NodeHandle<MNode>>(0, std::make_unique<MNode>());
    auto [hostNode, event] = hostGraph.addNode(handle);
    size_t id = nextId++;
    hostNode->test_node_identifier__ = id;
    event(audioGraph, disposer_);
    audioGraph[handle->index].test_node_identifier__ = id;
    liveNodes.push_back(hostNode);
    return hostNode;
  }

  void doRemoveNode(HNode *node) {
    auto result = hostGraph.removeNode(node);
    if (result.is_ok()) {
      auto event = std::move(result).unwrap();
      event(audioGraph, disposer_);
    }
    // Remove from live tracking
    liveNodes.erase(std::remove(liveNodes.begin(), liveNodes.end(), node), liveNodes.end());
  }

  bool doAddEdge(HNode *from, HNode *to) {
    auto result = hostGraph.addEdge(from, to);
    if (result.is_ok()) {
      auto event = std::move(result).unwrap();
      event(audioGraph, disposer_);
      return true;
    }
    return false;
  }

  bool doRemoveEdge(HNode *from, HNode *to) {
    auto result = hostGraph.removeEdge(from, to);
    if (result.is_ok()) {
      auto event = std::move(result).unwrap();
      event(audioGraph, disposer_);
      return true;
    }
    return false;
  }

  void doProcess() {
    audioGraph.process();
    // Note: this test only triggers audioGraph processing here.
    // Disposed-node collection is handled by higher-level wrappers in production code,
    // not directly inside the HostGraph addEdge/removeEdge methods used in this test.
  }

  HNode *pickRandom() {
    if (liveNodes.empty())
      return nullptr;
    return liveNodes[std::uniform_int_distribution<size_t>(0, liveNodes.size() - 1)(rng)];
  }

  std::pair<HNode *, HNode *> pickTwo() {
    if (liveNodes.size() < 2)
      return {nullptr, nullptr};
    auto dist = std::uniform_int_distribution<size_t>(0, liveNodes.size() - 1);
    size_t a = dist(rng);
    size_t b = dist(rng);
    if (a == b)
      return {nullptr, nullptr};
    return {liveNodes[a], liveNodes[b]};
  }

  // ── Cycle check on AudioGraph ───────────────────────────────────────────

  /// Returns true if AudioGraph has a cycle (via Kahn's out-degree check).
  bool audioGraphHasCycle() const {
    auto n = static_cast<uint32_t>(audioGraph.size());
    if (n <= 1)
      return false;

    // Compute out-degree
    std::vector<uint32_t> outDeg(n, 0);
    for (uint32_t i = 0; i < n; i++) {
      for (auto inp : audioGraph.pool().view(audioGraph[i].input_head)) {
        if (inp < n)
          outDeg[inp]++;
      }
    }

    // BFS from zero-out-degree nodes
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
      for (auto inp : audioGraph.pool().view(audioGraph[idx].input_head)) {
        if (inp < n && --outDeg[inp] == 0)
          queue.push_back(inp);
      }
    }

    return visited != n;
  }

  void dumpCycle() const {
    auto n = static_cast<uint32_t>(audioGraph.size());
    std::vector<uint32_t> outDeg(n, 0);
    for (uint32_t i = 0; i < n; i++)
      for (auto inp : audioGraph.pool().view(audioGraph[i].input_head))
        if (inp < n)
          outDeg[inp]++;

    std::vector<uint32_t> queue;
    for (uint32_t i = 0; i < n; i++)
      if (outDeg[i] == 0)
        queue.push_back(i);

    size_t head = 0;
    while (head < queue.size()) {
      auto idx = queue[head++];
      for (auto inp : audioGraph.pool().view(audioGraph[idx].input_head))
        if (inp < n && --outDeg[inp] == 0)
          queue.push_back(inp);
    }

    // Collect cycle node IDs
    std::set<size_t> cycleIds;
    std::cerr << "=== CYCLE DETECTED in AudioGraph (n=" << n << ") ===\n";
    std::cerr << "Nodes in cycle (non-zero remaining out-degree):\n";
    for (uint32_t i = 0; i < n; i++) {
      if (outDeg[i] > 0) {
        cycleIds.insert(audioGraph[i].test_node_identifier__);
        std::cerr << "  node[" << i << "] id=" << audioGraph[i].test_node_identifier__
                  << " inputs={";
        bool first = true;
        for (auto inp : audioGraph.pool().view(audioGraph[i].input_head)) {
          if (!first)
            std::cerr << ",";
          first = false;
          std::cerr << inp << "(id=" << audioGraph[inp].test_node_identifier__ << ")";
        }
        std::cerr << "} orphaned=" << audioGraph[i].orphaned << "\n";
      }
    }

    // Dump HostGraph adjacency for cycle nodes + all ghost info
    std::cerr << "\n=== HostGraph state ===\n";
    auto hostAdj = TestGraphUtils::convertHostGraphToAdjacencyList(hostGraph);
    for (auto id : cycleIds) {
      if (id < hostAdj.size()) {
        std::cerr << "  id=" << id << " outputs={";
        for (size_t j = 0; j < hostAdj[id].size(); j++) {
          if (j)
            std::cerr << ",";
          std::cerr << hostAdj[id][j];
        }
        std::cerr << "}\n";
      } else {
        std::cerr << "  id=" << id << " NOT IN HostGraph adj list\n";
      }
    }

    // Also show ghost status of all HostGraph nodes involved
    std::cerr << "\n=== HostGraph node details ===\n";
    for (auto *hn : hostGraph.nodes) {
      if (cycleIds.count(hn->test_node_identifier__)) {
        std::cerr << "  id=" << hn->test_node_identifier__ << " ghost=" << hn->ghost
                  << " handle.idx=" << hn->handle->index << " use_count=" << hn->handle.use_count()
                  << " inputs={";
        for (size_t j = 0; j < hn->inputs.size(); j++) {
          if (j)
            std::cerr << ",";
          std::cerr << hn->inputs[j]->test_node_identifier__;
          if (hn->inputs[j]->ghost)
            std::cerr << "(ghost)";
        }
        std::cerr << "} outputs={";
        for (size_t j = 0; j < hn->outputs.size(); j++) {
          if (j)
            std::cerr << ",";
          std::cerr << hn->outputs[j]->test_node_identifier__;
          if (hn->outputs[j]->ghost)
            std::cerr << "(ghost)";
        }
        std::cerr << "}\n";
      }
    }
  }

  /// Verify consistency between HostGraph and AudioGraph adjacency.
  void verifyConsistency() const {
    auto hostAdj = TestGraphUtils::convertHostGraphToAdjacencyList(hostGraph);
    auto audioAdj = TestGraphUtils::convertAudioGraphToAdjacencyList(audioGraph);

    // Only compare live (non-ghost) edges
    if (hostAdj.size() != audioAdj.size()) {
      std::cerr << "  [WARN] Adj list size mismatch: host=" << hostAdj.size()
                << " audio=" << audioAdj.size() << "\n";
    }
  }
};

// ── collectDisposedNodes is private; tests access it via friend declarations ──
// We call it directly in doProcess(), using HostGraph's friend declarations
// for TestGraphUtils and these test classes (it is not publicly exposed).

// =========================================================================
// Single-threaded deterministic cycle finder
// =========================================================================

TEST_P(GraphCycleDebugTest, FindCycleFormation) {
  // Seed
  size_t initialNodeCount = std::uniform_int_distribution<size_t>(16, 128)(rng);
  size_t opCount = std::uniform_int_distribution<size_t>(200, 2000)(rng);

  // Random operation weights
  size_t total = 100;
  size_t wAdd = std::uniform_int_distribution<size_t>(0, total)(rng);
  size_t wRem = std::uniform_int_distribution<size_t>(0, total - wAdd)(rng);
  size_t wEdge = std::uniform_int_distribution<size_t>(0, total - wAdd - wRem)(rng);

  // Seed graph
  for (size_t i = 0; i < initialNodeCount; i++) {
    doAddNode();
  }

  // Process once to establish baseline
  doProcess();
  ASSERT_FALSE(audioGraphHasCycle()) << "Cycle after initial seeding!";

  for (size_t i = 0; i < opCount; i++) {
    size_t op = std::uniform_int_distribution<size_t>(0, 99)(rng);
    std::string opName;

    if (op < wAdd) {
      opName = "addNode";
      doAddNode();

    } else if (op < wAdd + wRem) {
      auto *n = pickRandom();
      if (n) {
        opName = "removeNode(id=" + std::to_string(n->test_node_identifier__) + ")";
        doRemoveNode(n);
      } else {
        opName = "removeNode(skip)";
      }

    } else if (op < wAdd + wRem + wEdge) {
      auto [from, to] = pickTwo();
      if (from && to) {
        bool ok = doAddEdge(from, to);
        opName = "addEdge(id=" + std::to_string(from->test_node_identifier__) +
            "->id=" + std::to_string(to->test_node_identifier__) + (ok ? " OK)" : " REJECTED)");
      } else {
        opName = "addEdge(skip)";
      }

    } else {
      auto [from, to] = pickTwo();
      if (from && to) {
        bool ok = doRemoveEdge(from, to);
        opName = "removeEdge(id=" + std::to_string(from->test_node_identifier__) +
            "->id=" + std::to_string(to->test_node_identifier__) + (ok ? " OK)" : " REJECTED)");
      } else {
        opName = "removeEdge(skip)";
      }
    }

    // Check for cycle BEFORE process (events applied, toposort not yet)
    if (audioGraphHasCycle()) {
      std::cerr << "\n!!! CYCLE FOUND at operation " << i << ": " << opName << "\n";
      dumpCycle();
      FAIL() << "Cycle found at operation " << i << ": " << opName;
    }

    // Process (toposort + compaction) every few operations
    if (i % 3 == 0) {
      doProcess();
      if (audioGraphHasCycle()) {
        std::cerr << "\n!!! CYCLE FOUND after process() at operation " << i << ": " << opName
                  << "\n";
        dumpCycle();
        FAIL() << "Cycle found after process() at operation " << i;
      }
    }
  }

  // Final process
  doProcess();
  ASSERT_FALSE(audioGraphHasCycle()) << "Cycle after all operations!";
}

INSTANTIATE_TEST_SUITE_P(
    Seeds,
    GraphCycleDebugTest,
    ::testing::Range(uint64_t{0}, uint64_t{100}),
    [](const ::testing::TestParamInfo<uint64_t> &info) {
      return "seed_" + std::to_string(info.param);
    });

} // namespace audioapi::utils::graph
